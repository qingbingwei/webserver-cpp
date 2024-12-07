#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

class http_conn
{
public:
	//文件名大小------real_file
	static const int FILENAME_MAX_LEN = 200;
	//读缓冲区大小-------read_buf
	static const int READ_BUFFER_SIZE = 2048;
	//写缓冲区大小-------write_buf
	static const int WRITE_BUFFER_SIZE = 1024;
	//报文的请求方法,只使用GET和POST
	enum METHOD{GET=0,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,CONNECT,PATH};
	//主状态机状态
	enum CHECK_STATE{CHECK_STATE_REQUESTLINE=0,CHECK_STATE_HEADER,CHECK_STATE_CONNECT};
	//报文解析结果
	enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};
	//从状态机状态
	enum LINE_STATUS{LINE_OK=0,LINE_BAD,LINE_OPEN};

public:
	http_conn();
	~http_conn();

public:
	//初始化套接字地址，内部再调用私有方法init
	void init(int sockfd,const sockaddr_in& addr);
	//关闭http连接
	void close_http_conn(bool real_close=true);
	//调用process_read函数和process_write函数分别完成报文解析与报文响应两个任务
	void process();
	//读取浏览器发送的全部数据
	bool read_once();
	//将响应报文写入
	bool write();

	sockaddr_in* get_addr();

	//同步线程初始化数据库读取表

	//CGI使用线程池初始化数据库表

private:
	void init();
	//从read_buf读取数据，并处理请求报文
	HTTP_CODE process_read();
	//向write_buf写入响应报文数据
	bool process_write(HTTP_CODE ret);
	//主状态机解析请求行
	HTTP_CODE parse_request_line(char* text);
	//主状态机解析请求头
	HTTP_CODE parse_headers(char* text);
	//主状态机解析请求体
	HTTP_CODE parse_content(char* text);
	//生成响应报文
	HTTP_CODE do_request();

	//start_line为已经解析的字符
	//get_line用于将指针向后偏移，指向未处理字符
	char* get_line();

	//从状态机读取一行，分析是请求报文哪一部分
	LINE_STATUS parse_line();

	void unmap();

	//根据响应报文格式，生成报文对应8个部分，均由do_request调用
	bool add_response(const char* format,...);
	bool add_content(const char* content);
	bool add_status_line(int status,const char* title);
	bool add_headers(int content_length);
	bool add_content_type();
	bool add_content_length(int content_length);
	bool add_linger();
	bool add_blank_line();

public:
	static int epoll_fd;
	static int user_count;
	/*MYSQL* mysql*/

private:
	int sock_fd;
	sockaddr_in address;

	//存储读取的请求报文数据
	char read_buf[READ_BUFFER_SIZE];
	//缓冲区中read_buf最后一个数据的下一个位置
	int read_idx;
	//read_buf读取的位置
	int checked_idx;
	//read_buf已经解析的字符个数
	int start_line;

	//存储发出的响应报文数据
	char write_buf[WRITE_BUFFER_SIZE];
	//指示buf中的长度
	int write_idx;

	//主状态机的状态
	CHECK_STATE check_state;
	//请求方法
	METHOD method;

	//以下为解析请求报文中对应的6个变量
	//存储读取文件的名称
	char real_file[FILENAME_MAX_LEN];
	char* url;
	char* version;
	char* host;
	int content_length;
	//控制套接字在关闭时是否等待发送剩余数据
	bool linger;

	char* file_address; //读取服务器上文件地址
	
	struct stat file_stat;
	struct iovec iv[2]; //io向量机制iovec
	int iv_count;
	int cgi; //是否启用的POST
	char* header_string; //存储请求头数据
	int bytes_to_send; //剩余发送字节数
	int bytes_have_send; //已发送字节数

	
};


#endif
