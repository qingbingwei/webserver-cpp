#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <list>
#include "lock_sem_cond.h"

template <class T>
class Thread_Pool
{
private:
	int thread_number; // 线程池中的线程数

	int max_requests; // 请求队列允许的最大请求数

	pthread_t* pool; // 线程池数组

	std::list<T*> work_queue; // 请求队列

	Lock queue_lock; // 保护请求队列的互斥锁

	sem queue_stat; // 是否有任务需要处理

	bool stop_thread; // 是否结束进程

	/*Connection_Pool* conn_pool*/ // 数据库连接池

private:
	// 工作线程调用的函数
	// 从工作队列不断取出任务并执行
	static void* worker(void* arg);

	void run();

public:
	// thread_number是线程池中线程的数量
	// max_requests是请求队列中最多允许的、等待处理的请求的数量
	// connPool是数据库连接池指针
	Thread_Pool(/*Connection_Pool* Conn_Pool*/ int Thread_num = 8, int Max_requests = 10000);
	~Thread_Pool();

	bool append(T* request);
	// 向请求队列添加任务
};

#endif