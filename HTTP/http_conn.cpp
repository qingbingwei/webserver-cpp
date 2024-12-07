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
}

void remove_fd()
{

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
	HTTP_CODE read_ret = process_read();

	//NO_REQUEST，表示请求不完整，需要继续接收请求数据
	if(read_ret == NO_REQUEST)
	{
		//注册监听读事件
		
	}
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