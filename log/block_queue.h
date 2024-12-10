/*************************************************************
*循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;  
*线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
**************************************************************/


#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <exception>
#include <time.h>
#include <sys/time.h>
#include "../lock_sem_cond/lock_sem_cond.h"

template<class T>
class block_queue
{
public:
	block_queue(int m_max_size = 1000);
	~block_queue();

	void clear();

	//判断队列是否满了
	bool is_full();

	//判断队列是否为空
	bool empty();

	//返回队首元素
	bool get_front(T& value);

	//返回队尾元素
	bool get_back(T& value);

	int get_size();

	int get_max_size();

	//往队列添加元素，需要将所有使用队列的线程先唤醒
	//当有元素push进队列，相当于生产者生产了一个元素
	//若当前没有线程等待条件变量,则唤醒无意义
	bool push(const T& item);

	//pop时，如果当前队列没有元素,将会等待条件变量
	bool pop(T& item);

	//在上面基础上增加了超时处理，但是没用到
	//在pthread_cond_wait基础上增加了等待的时间，只指定时间内能抢到互斥锁即可
	//其他逻辑不变
	bool pop(T& item,int ms_timeout);

private:
	Lock mutex;
	Cond cond;

	T* array;
	int size;
	int max_size;
	int front;
	int back;
};


#endif