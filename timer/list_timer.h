#ifndef LIST_TIMER_H
#define LIST_TIMER_H

#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <time.h>

#include "../HTTP/http_conn.h"

//连接资源结构体成员需要用到定时器类
//需要前向声明
class util_timer;

//连接资源
struct client_data
{
	//客户端socket地址
	sockaddr_in address;

	//socket文件描述符
	int sock_fd;

	//定时器
	util_timer* timer;
};

//将连接资源、定时事件和超时时间封装为定时器类
//定时器类
class util_timer
{
public:
	util_timer();

public:
	//超时时间
	time_t expire;
	//回调函数
	void (*cb_func)(client_data*);
	//连接资源
	client_data* user_data;
	//前向定时器
	util_timer* prev;
	//后继定时器
	util_timer* next;
};

//定时器容器类
class sort_timer_list
{
public:
	sort_timer_list();
	//销毁链表
	~sort_timer_list();

	//添加定时器，内部调用私有成员add_timer
	void add_timer(util_timer* timer);

	//调整定时器，任务发生变化时，调整定时器在链表中的位置
	void adjust_timer(util_timer* timer);

	//删除定时器
	void del_timer(util_timer* timer);
	
	//定时任务处理函数
	void tick();

private:
	//私有成员，被公有成员add_timer和adjust_time调用
	//主要用于调整链表内部结点
	void add_timer(util_timer* timer,util_timer* list_head);

private:
	//头尾节点
	util_timer* head;
	util_timer* tail;
};

class Utils
{
public:
	Utils();
	~Utils();

	void init(int time_slot);

	//对文件描述符设置非阻塞
	int set_non_blocking(int fd);

	//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
	void add_fd(int epoll_fd,int fd,bool one_shot,bool ET_Mode);

	//信号处理函数----要不要static
	void sig_handler(int sig);

	//设置信号函数
	void add_sig(int sig,void(handler)(int),bool restart = true);

	//定时处理任务，重新定时以不断触发SIGALRM信号
	void timer_handler();

	void show_error(int conn_fd,const char* info);

public:
	int* pipe_fd;
	static int epoll_fd;
	int TIMESLOT;
	sort_timer_list timer_list;
};

void cb_func(client_data *user_data);

#endif