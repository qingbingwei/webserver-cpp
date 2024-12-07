#include "lock_sem_cond.h"

Lock::Lock()
{
	if(pthread_mutex_init(&mutex,NULL) != 0)
	{
		throw std::exception();
	}
}

Lock::~Lock()
{
	if(pthread_mutex_destroy(&mutex) != 0)
	{
		throw std::exception();
	}
}

bool Lock::lock()
{
	if(pthread_mutex_lock(&mutex) == 0)
	{
		return true;
	}
	return false;
}
//上锁

bool Lock::unlock()
{
	if(pthread_mutex_unlock(&mutex) == 0)
	{
		return true;
	}
	return false;
}
//解锁

pthread_mutex_t* Lock::get_lock()
{
	return &mutex;
}
//获取锁


Sem::Sem()
{
	if(sem_init(&sem,0,0) == -1)
	{
		throw std::exception();
	}
}

Sem::Sem(int num)
{
	if(sem_init(&sem,0,num) == -1)
	{
		throw std::exception();
	}
}

Sem::~Sem()
{
	if(sem_destroy(&sem) == -1)
	{
		throw std::exception();
	}
}

bool Sem::P()
{
	if(sem_wait(&sem) == 0)
	{
		return true;
	}
	return false;
}
//申请资源操作

bool Sem::V()
{
	if(sem_post(&sem) == 0)
	{
		return true;
	}
	return false;
}
//释放资源操作

Cond::Cond()
{
	if(pthread_cond_init(&cond,NULL) != 0)
	{
		throw std::exception();
	}
}

Cond::~Cond()
{
	if(pthread_cond_destroy(&cond) != 0)
	{
		throw std::exception();
	}
}

bool Cond::wait(pthread_mutex_t* mutex)
{
	if(pthread_cond_wait(&cond,mutex) != 0)
	{
		return true;
	}
	return false;
}
//没有超时机制

bool Cond::time_wait(pthread_mutex_t* mutex,const timespec time)
{
	if(pthread_cond_timedwait(&cond,mutex,&time) != 0)
	{
		return true;
	}
	return false;
}
//有超时机制

bool Cond::wake_up()
{
	if(pthread_cond_signal(&cond) != 0)
	{
		return true;
	}
	return false;
}
//唤醒第一个消费者线程

bool Cond::wake_up_all()
{
	if(pthread_cond_broadcast(&cond) != 0)
	{
		return true;
	}
	return false;
}
//唤醒所有消费者线程