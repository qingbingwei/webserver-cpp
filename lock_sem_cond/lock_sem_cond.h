#ifndef LOCK_SEM_COND_H
#define LOCK_SEM_COND_H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

class Lock
{
private:
	pthread_mutex_t mutex;
public:
	Lock();
	~Lock();
	bool lock();
	//上锁
	bool unlock();
	//解锁
	pthread_mutex_t* get_lock();
	//获取锁
};

class Sem
{
private:
	sem_t sem;
public:
	Sem();
	Sem(int num);
	~Sem();
	bool P();
	//申请资源操作
	bool V();
	//释放资源操作
};

class Cond
{
private:
	pthread_cond_t cond;
public:
	Cond();
	~Cond();
	bool wait(pthread_mutex_t* mutex);//没有超时机制
	bool time_wait(pthread_mutex_t* mutex,const timespec time);//有超时机制
	bool wake_up();//唤醒第一个消费者线程
	bool wake_up_all();//唤醒所有消费者线程
};


#endif