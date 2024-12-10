#include "http_conn.h"


//定义http响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the request file.\n";


//对文件描述符设置非阻塞
int set_non_blocking(int fd)
{
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
	return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void add_fd(int epoll_fd,int fd,bool one_shot,bool ET_Mode)
{
	epoll_event event;
	event.data.fd = fd;

	if(ET_Mode == true)
	{
		//ET模式
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	}
	else
	{
		//LT模式
		event.events = EPOLLIN | EPOLLRDHUP;
	}

	//是否开启EPOLLONESHOT
	if(one_shot == true)
	{
		event.events |= EPOLLONESHOT;
	}

	epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
	set_non_blocking(fd);
}

//从内核事件表删除描述符
void remove_fd(int epoll_fd,int fd)
{
	epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,NULL);
	close(fd);
}

//将事件重置为EPOLLONESHOT
void mod_fd(int epoll_fd,int fd,int old_event,bool ET_Mode)
{
	epoll_event event;
	event.data.fd = fd;

	if(ET_Mode == true)
	{
		//ET模式
		event.events = old_event | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
	}
	else
	{
		//LT模式
		event.events = old_event | EPOLLONESHOT | EPOLLRDHUP;
	}

	epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&event);
}

http_conn::http_conn()
{

}

http_conn::~http_conn()
{

}

void http_conn::init(int sockfd,const sockaddr_in& addr)
{
	this->init();
}

void http_conn::close_http_conn(bool real_close=true)
{

}

void http_conn::process()
{
	//调用process_read完成报文解析
	HTTP_CODE read_ret = process_read();

	//NO_REQUEST，表示请求不完整，需要继续接收请求数据
	if(read_ret == NO_REQUEST)
	{
		//再次注册监听读事件
		mod_fd(epoll_fd,sock_fd,EPOLLIN,true);
		return;
	}

	//调用process_write完成报文响应
	bool write_ret = process_write(read_ret);
	if(!write_ret)
	{
		close_http_conn();
	}
	//再次注册并监听写事件
	mod_fd(epoll_fd,sock_fd,EPOLLOUT,true);
}

//start_line是行在buffer中的起始位置，将该位置后面的数据赋给text
//此时从状态机已提前将一行的末尾字符\r\n变为\0\0
//所以text可以直接取出完整的行进行解析
char* http_conn::get_line()
{
	return read_buf + start_line;
}

http_conn::HTTP_CODE http_conn::process_read()
{
	//初始化从状态机的状态与http请求解析结果
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE read_ret = NO_REQUEST;
	char* text = NULL;

	//parse_line为从状态机的具体实现
	while((check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
	{
		text = get_line();

		//start_line是每一个数据行在read_buf中的起始位置
		//checked_idx表示从状态机在read_buf中读取的位置
		start_line = checked_idx;

		//主状态机三种状态转移
		if(check_state == CHECK_STATE_REQUESTLINE)
		{
			//解析请求行
			read_ret = parse_request_line(text);
			if(read_ret == BAD_REQUEST)
			{
				return BAD_REQUEST;
			}
		}
		else if(check_state == CHECK_STATE_HEADER)
		{
			//解析请求头
			read_ret = parse_headers(text);
			if(read_ret == BAD_REQUEST)
			{
				return BAD_REQUEST;
			}
			else if(read_ret == GET_REQUEST)
			{
				//完整解析GET请求后，跳转到报文响应函数
				return do_request();
			}
		}
		else if(check_state == CHECK_STATE_CONTENT)
		{
			//解析请求体
			read_ret = parse_content(text);

			//完整解析POST请求后，跳转到报文响应函数
			if(read_ret == GET_REQUEST)
			{
				return do_request();
			}
			//解析完消息体即完成报文解析，避免再次进入循环，更新line_status
			line_status = LINE_OPEN;
		}
		else
		{
			return INTERNAL_ERROR;
		}
	}
	return NO_REQUEST;
}

//从状态机，用于分析出一行内容
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN

//read_idx指向缓冲区read_buf的数据末尾的下一个字节
//checked_idx指向从状态机当前正在分析的字节
http_conn::LINE_STATUS http_conn::parse_line()
{
	char tmp = '\0';
	for(;checked_idx < read_idx;++checked_idx)
	{
		//tmp为将要分析的字节
		tmp = read_buf[checked_idx];

		//如果当前是\r,可能读到完整行
		if(tmp == '\r')
		{
			if((checked_idx + 1) == read_idx)
			{
				//下一个字符达到了buffer结尾，则接收不完整，需要继续接收
				return LINE_OPEN;
			}
			else if(read_buf[checked_idx + 1] == '\n')
			{
				//下一个字符是\n，将\r\n改为\0\0
				read_buf[checked_idx] = '\0';
				++checked_idx;
				read_buf[checked_idx] = '\0';
				++checked_idx;
				return LINE_OK;
			}
			else
			{
				//都不符合，则返回语法错误
				return LINE_BAD;
			}
		}
		else if(tmp == '\n')
		{
			//如果当前字符是\n，也有可能读取到完整行
			//一般是上次读取到\r就到buffer末尾了，没有接收完整，再次接收时会出现这种情况
			
			//前一个是'\r',说明接收完整

			//为什么不取等？
			if(checked_idx > 1 && read_buf[checked_idx - 1] == '\r')
			{
				read_buf[checked_idx - 1] = '\0';
				read_buf[checked_idx] = '\0';
				++checked_idx;
				return LINE_OK;
			}
			else
			{
				return LINE_BAD;
			}
		}
	}
	
	//没有\r\n,继续接收
	return LINE_OPEN;
}

//解析http请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char* text)
{
	//在HTTP报文中，请求行用来说明请求类型,要访问的资源以及所使用的HTTP版本
	//其中各个部分之间通过\t或空格分隔
	
	//请求行中最先含有空格和\t任一字符的位置并返回
	url = strpbrk(text," \t");

	//如果没有空格或\t，说明格式错误
	if(url == NULL)
	{
		return BAD_REQUEST;
	}

	//将该位置改为\0，用于将前面数据取出
	*url = '\0';
	++url;

	//取出数据，并比较请求方式，确定是GET还是POST
	char* request_method = text;
	if(strcasecmp(request_method,"GET") == 0)
	{
		method = GET;
	}
	else if(strcasecmp(request_method,"POST") == 0)
	{
		method = POST;
		cgi = true;
	}
	else
	{
		return BAD_REQUEST;
	}

	//url此时跳过了第一个空格或\t字符，但不知道之后是否还有
	//将url向后偏移，通过查找，继续跳过空格和\t字符
	//最终指向请求资源的第一个字符

	url += strspn(url," \t");

	//判断http版本号，方法同上
	version = strpbrk(url," \t");

	if(version == NULL)
	{
		return BAD_REQUEST;
	}

	//将该位置改为\0，用于将前面数据取出
	*version = '\0';
	++version;

	version += strspn(version," \t");

	//只支持HTTP/1.1
	if(strcasecmp(version,"HTTP/1.1") != 0)
	{
		return BAD_REQUEST;
	}

	//处理请求资源
	//对请求资源前7个字符进行判断
	//主要是有些报文的请求资源中会带有http://，这里需要对这种情况进行单独处理
	if(strncasecmp(url,"http://",7) == 0)
	{
		url += 7;
		url = strchr(url,'/');
	}

	//https
	if(strncasecmp(url,"https://",7) == 0)
	{
		url += 8;
		url = strchr(url,'/');
	}

	//一般的不会带有上述两种符号，直接是单独的/或/后面带访问资源
	if(url == NULL || url[0] != '/')
	{
		return BAD_REQUEST;
	}

	//当url为/时，显示欢迎界面
	if(strlen(url) == 1)
	{
		strcat(url,"index.html");
	}

	//请求行处理完毕，将主状态机转移处理请求头
	check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

//解析http请求的头部信息
//解析空行也复用该函数
http_conn::HTTP_CODE http_conn::parse_headers(char* text)
{
	//判断是头部还是空行
	if(text[0] == '\0')
	{
		//空行
		//判断请求类型(GET or POST)
		if(content_length != 0)
		{
			//POST需要跳转到消息体处理状态
			check_state = CHECK_STATE_CONTENT;
			return BAD_REQUEST;
		}
		else
		{
			return GET_REQUEST;
		}
	}
	else if(strncasecmp(text,"Connection:",11) == 0)
	{
		//解析请求头连接字段
		text += 11;

		//跳过空格和\t
		text += strspn(text," \t");
		if(strcasecmp(text,"keep-alive") == 0)
		{
			//如果是长连接，则将linger标志设置为true
			linger = true;
		}
	}
	else if(strncasecmp(text,"Content-length:",15) == 0)
	{
		//解析请求头内容长度字段
		text += 15;

		text += strspn(text," \t");
		content_length = atol(text);
	}
	else if(strncasecmp(text,"Host:",5) == 0)
	{
		//解析请求头HOST字段
		text += 5;
		
		text += strspn(text," \t");
		host = text;
	}
	else 
	{
		printf("oop!unknow header : %s\n",text);
	}
	return NO_REQUEST;
}

//判断http请求是否被完整读入
//解析http请求体
http_conn::HTTP_CODE http_conn::parse_content(char* text)
{
	//判断read_buf中是否读取了消息体
	if(read_idx >= (content_length + checked_idx))
	{
		text[content_length] = '\0';

		//POST请求中最后为用户名和密码
		m_string = text;

		return GET_REQUEST;
	}
	return NO_REQUEST;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
bool http_conn::read_once()
{
	if(read_idx >= READ_BUFFER_SIZE)
	{
		return false;
	}
	
	int bytes_read(0);

	while(true)
	{
		//从套接字接收数据，存储到read_buf
		bytes_read = recv(sock_fd,read_buf + read_idx,READ_BUFFER_SIZE - read_idx,0);
		if(bytes_read == -1)
		{
			//非阻塞ET模式下，需要一次性将数据读完
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break;
			}
			return -1;
		}
		else if(bytes_read == 0)
		{
			return false;
		}
		//更新read_idx
		read_idx += bytes_read;
	}
	return true;
}

http_conn::HTTP_CODE http_conn::do_request()
{
	//将初始化的real_file赋值为网站根目录
	strcpy(real_file,doc_root);
	int len = strlen(doc_root);

	//找到url中/的位置
	const char* p = strrchr(url,'/');

	//实现登录和注册校验
	//if()
	//{
		//根据标志判断是登录检测还是注册检测

		//同步线程登录校验

		//CGI多进程登录校验
	//}

	//如果请求资源为/0，表示跳转注册界面
	if(*(p+1) == '0')
	{
		char* real_url = new char[200];
		strcpy(real_url,"/register.html");

		//将网站目录和/register.html进行拼接，更新到real_file中
		strncpy(real_file+len,real_url,strlen(real_url));

		delete[] real_url;
		real_url = NULL;
	}
	else if(*(p+1) == '1')
	{
		//如果请求资源为/1，表示跳转登录界面
		char* real_url = new char[200];
		strcpy(real_url,"/login.html");

		//将网站目录和/login.html进行拼接，更新到real_file中
		strncpy(real_file+len,real_url,strlen(real_url));

		delete[] real_url;
		real_url = NULL;
	}
	else
	{
		//如果以上均不符合，即不是登录和注册，直接将url与网站目录拼接
		//这里的情况是welcome界面，请求服务器上的一个图片
		strncpy(real_file + len,url,FILENAME_MAX_LEN - len - 1);
	}

	//通过stat获取请求资源文件信息，成功则将信息更新到file_stat结构体
	//失败返回NO_RESOURCE状态，表示资源不存在
	if(stat(real_file,&file_stat) == -1)
	{
		perror("get file status errno");
		return NO_RESOURCE;
	}

	//判断文件的权限，是否可读，不可读则返回FORBIDDEN_REQUEST状态
	if(!(file_stat.st_mode & S_IROTH))
	{
		return FORBIDDEN_REQUEST;
	}

	//判断文件类型，如果是目录，则返回BAD_REQUEST，表示请求报文有误
	if(S_ISDIR(file_stat.st_mode))
	{
		return BAD_REQUEST;
	}

	//以只读方式获取文件描述符，通过mmap将该文件映射到内存中
	int fd = open(real_file,O_RDONLY);
	if(fd == -1)
	{
		perror("open errno");
		return BAD_REQUEST;
	}

	file_address = (char*)mmap(0,file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0);

	//关闭文件描述符
	close(fd);

	//表示请求文件存在，且可以访问
	return FILE_REQUEST;
}

bool http_conn::add_response(const char* format,...)
{
	//如果写入内容超出m_write_buf大小则报错
	if(write_idx >= WRITE_BUFFER_SIZE)
	{
		return false;
	}

	//定义可变参数列表
	va_list arg_list;

	//将变量arg_list初始化为传入参数
	va_start(arg_list,format);

	//将数据format从可变参数列表写入缓冲区，返回写入数据的长度
	int len = vsnprintf(write_buf+write_idx,WRITE_BUFFER_SIZE - write_idx - 1,format,arg_list);

	//如果写入的数据长度超过缓冲区剩余空间，则报错
	if(len >= (WRITE_BUFFER_SIZE - write_idx - 1))
	{
		va_end(arg_list);
		return false;
	}

	//更新write_idx
	write_idx += len;
	//清空可变参列表
	va_end(arg_list);

	return true;
}

//添加状态行
bool http_conn::add_status_line(int status,const char* title)
{
	return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}

//添加消息报头，具体的添加文本长度、连接状态和空行
bool http_conn::add_headers(int content_length)
{
	add_content_length(content_length);
	add_linger();
	add_blank_line();
}

//添加Content-Length，表示响应报文的长度
bool http_conn::add_content_length(int content_length)
{
	return add_response("Content-Length:%d\r\n",content_length);
}

//添加文本类型，这里是html
bool http_conn::add_content_type()
{
	return add_response("Content-Type:%s\r\n","text/html");
}

//添加连接状态，通知浏览器端是保持连接还是关闭
bool http_conn::add_linger()
{
	return add_response("Connection:%s\r\n",(linger == true) ? "keep-alive" : "close");
}

//添加空行
bool http_conn::add_blank_line()
{
	return add_response("%s","\r\n");
}

//添加文本content
bool http_conn::add_content(const char* content)
{
	return add_response("%s",content);
}

bool http_conn::process_write(HTTP_CODE ret)
{
	if(ret == INTERNAL_ERROR)
	{
		//内部错误，500
		//状态行
		add_status_line(500,error_500_title);
		//消息报头
		add_headers(strlen(error_500_form));
		if(!add_content(error_500_form))
		{
			return false;
		}
	}
	else if(ret == BAD_REQUEST)
	{
		//报文语法有误，404
		//状态行
		add_status_line(404,error_404_title);
		//消息报头
		add_headers(strlen(error_404_form));
		if(!add_content(error_404_form))
		{
			return false;
		}
	}
	else if(ret == FORBIDDEN_REQUEST)
	{
		//资源没有访问权限，403
		//状态行
		add_status_line(403,error_403_title);
		//消息报头
		add_headers(strlen(error_403_form));
		if(!add_content(error_403_form))
		{
			return false;
		}
	}
	else if(ret == FILE_REQUEST)
	{
		//文件存在，200
		add_status_line(200,ok_200_title);
		//如果资源存在
		if(file_stat.st_size != 0)
		{
			add_headers(file_stat.st_size);
			//第一个iovec指针指向响应报文缓冲区，长度指向write_idx
			iv[0].iov_base = write_buf;
			iv[0].iov_len = write_idx;
			//第二个iovec指针指向mmap返回的文件指针，长度指向文件大小
			iv[1].iov_base = file_address;
			iv[1].iov_len = file_stat.st_size;
			iv_count = 2;
			//发送的全部数据为响应报文头部信息和文件大小
			bytes_to_send = write_idx + file_stat.st_size;
			return true;
		}
		else
		{
			//如果请求的资源大小为0，则返回空白html文件
			const char* ok_string = "<html><body></body></html>";
			add_headers(strlen(ok_string));
			if(!add_content(ok_string))
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}
	//除FILE_REQUEST状态外，其余状态只申请一个iovec，指向响应报文缓冲区
	iv[0].iov_base = write_buf;
	iv[0].iov_len = write_idx;
	iv_count = 1;
	return true;
}

bool http_conn::write()
{
	int tmp = 0;

	int newadd = 0;

	//若要发送的数据长度为0
    //表示响应报文为空，一般不会出现这种情况
	if(bytes_to_send == 0)
	{
		mod_fd(epoll_fd,sock_fd,EPOLLIN,true);
		init();
		return true;
	}

	while(true)
	{
		//将响应报文的状态行、消息头、空行和响应正文发送给浏览器端
		tmp = writev(sock_fd,iv,iv_count);

		//正常发送，temp为发送的字节数
		if(tmp > 0)
		{
			//更新已发送字节
			bytes_have_send += tmp;
			//偏移文件iovec的指针
			newadd = bytes_have_send - write_idx;
		}
		else if(tmp <= -1)
		{
			//判断缓冲区是否满了
			if(errno == EAGAIN)
			{
				//第一个iovec头部信息的数据已发送完，发送第二个iovec数据
				if(bytes_have_send >= iv[0].iov_len)
				{
					//不再继续发送头部信息
					iv[0].iov_len = 0;
					iv[1].iov_base = file_address + newadd;
					iv[1].iov_len = bytes_to_send;
				}
				else
				{
					//继续发送第一个iovec头部信息的数据
					iv[0].iov_base = write_buf + bytes_have_send;
					iv[0].iov_len = iv[0].iov_len - bytes_have_send;
				}
				//重新注册写事件
				mod_fd(epoll_fd,sock_fd,EPOLLOUT,true);
				return true;
			}
			else
			{
				//发送失败，但不是缓冲区问题，取消映射
				unmap();
				return false;
			}
		}

		//更新已发送字节数
		bytes_to_send -= tmp;

		//判断条件，数据已全部发送完
		if(bytes_to_send <= 0)
		{
			unmap();

			//在epoll树上重置EPOLLONESHOT事件
			mod_fd(epoll_fd,sock_fd,EPOLLIN,true);

			//浏览器的请求为长连接
			if(linger)
			{
				//重新初始化HTTP对象
				init();
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}
