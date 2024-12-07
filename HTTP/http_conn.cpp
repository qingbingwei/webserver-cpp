#include "http_conn.h"

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
	//
	while(true)
	{
		text = get_line();

		//start_line是每一个数据行在read_buf中的起始位置
		//checked_idx表示从状态机在read_buf中读取的位置
		start_line = checked_idx;

		//主状态机三种状态转移
		if(check_state == CHECK_STATE_REQUESTLINE)
		{
			//解析请求行

		}
		else if(check_state == CHECK_STATE_HEADER)
		{
			//解析请求头
			
		}
		else if(check_state == CHECK_STATE_CONNECT)
		{
			//解析请求体
			
		}
		else
		{

		}
	}
}

http_conn::HTTP_CODE http_conn::process_read()
{
	
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

bool http_conn::write()
{

}

sockaddr_in* http_conn::get_addr()
{

}