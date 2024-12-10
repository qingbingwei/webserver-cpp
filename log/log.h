#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

#include "block_queue.h"
#include "../lock_sem_cond/lock_sem_cond.h"

class Log
{
public:
	// C++11以后,使用局部变量懒汉不用加锁
	static Log* get_instance();

	//可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
	bool init(const char* file_name,int log_buf_size = 8192,int m_split_lines = 5000000,int max_queue_size = 0);

	//异步写日志公有方法，调用私有方法async_write_log
	static void* flush_log_thread(void* arg);

	//将输出内容按照标准格式整理
	void write_log(int level,const char* format,...);

	//强制刷新缓冲区
	void flush();

private:
	Log();
	virtual ~Log();

	//异步写日志方法
	void* async_write_log();

private:
	char dir_name[128]; //路径名
	char log_name[128]; //log文件名
	int split_lines; //日志最大行数
	int log_buf_size; //日志缓冲区大小
	long long count; //日志行数记录
	int today; //按天分文件,记录当前时间是那一天
	FILE* fp; //打开log的文件指针
	char* w_buf; //要输出的内容
	block_queue<std::string>* log_queue; //阻塞队列
	bool is_async; //是否同步标志位
	Lock mutex; //同步类
};

//这四个宏定义在其他文件中使用，主要用于不同类型的日志输出
#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__)

#endif