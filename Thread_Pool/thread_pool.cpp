#include "thread_pool.h"

template<class T>
Thread_Pool<T>::Thread_Pool(/*Connection_Pool* Conn_Pool*/ int Thread_num = 8, int Max_requests = 10000):thread_number(Thread_num),max_requests(Max_requests),stop_thread(false),pool(nullptr)/*conn_pool*/
{
	if(Thread_num <= 0 || Max_requests <= 0)
	{
		throw std::exception();
	}

	//线程id初始化
	pool = new pthread_t[thread_number];
	if(pool == nullptr)
	{
		throw std::exception();
	}
	for(int i = 0;i < thread_number;++i)
	{
		//创建线程，并且传入工作函数
		//pool为数组名，即指针
		if(pthread_create(pool+i,NULL,worker,this) != 0)
		{
			delete[] pool;
			throw std::exception();
		}

		//将线程设置为分离态,不用单独对工作线程进行回收
		if(pthread_detach(pool[i]) != 0)
		{
			delete[] pool;
			throw std::exception();
		}
	}
}

template<class T>
bool Thread_Pool<T>::append(T* request)
{
	queue_lock.lock();

	//设置请求队列的最大值
	if(work_queue.size() > max_requests)
	{
		queue_lock.unlock();
		return false;
	}

	//添加任务
	work_queue.push_back(request);
	queue_lock.unlock();

	//使用信号量提醒有任务要进行处理
	queue_stat.V();
	return true;
}

template<class T>
void* Thread_Pool<T>::worker(void* arg)
{
	//解析参数
	Thread_Pool* m_pool = (Thread_Pool*)arg;
	m_pool->run();
	return m_pool;
}

template<class T>
void Thread_Pool<T>::run()
{
	while(!stop_thread)
	{
		//等待信号量
		queue_stat.P();

		//唤醒后加上互斥锁
		queue_lock.lock();
		if(work_queue.empty())
		{
			queue_lock.unlock();
			continue;
		}

		//从队列中取出第一个任务
		//同时将任务从队列删除
		T* request = work_queue.front();
		work_queue.pop_front();
		queue_lock.unlock();
		if(!request)
		{
			continue;
		}

		//从连接池中取出一个数据库连接

		//process(模板类中的方法,这里是http类)进行处理

		//将数据库连接放回连接池
	}
}