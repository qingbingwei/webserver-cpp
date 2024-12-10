#include "block_queue.h"

template<class T>
block_queue<T>::block_queue(int m_max_size = 1000)
{
	if(m_max_size <= 0)
	{
		throw std::exception();
	}

	max_size = m_max_size;
	array = new T[max_size];
	size = 0;
	front = -1;
	back = -1;
}

template<class T>
block_queue<T>::~block_queue()
{
	mutex.lock();
	if(array != NULL)
	{
		delete[] array;
		array = NULL;
	}
	mutex.unlock();
}

template<class T>
void block_queue<T>::clear()
{
	mutex.lock();
	size = 0;
	front = -1;
	back = -1;
	mutex.unlock();
}

template<class T>
bool block_queue<T>::is_full()
{
	mutex.lock();
	if(size >= max_size)
	{
		mutex.unlock();
		return true;
	}
	else
	{
		mutex.unlock();
		return false;
	}
}

template<class T>
bool block_queue<T>::empty()
{
	mutex.lock();
	if(size == 0)
	{
		mutex.unlock();
		return true;
	}
	else
	{
		mutex.unlock();
		return false;
	}
}

template<class T>
bool block_queue<T>::get_front(T& value)
{
	mutex.lock();
	if(size == 0)
	{
		mutex.unlock();
		return false;
	}
	else
	{
		value = array[front];
		mutex.unlock();
		return true;
	}
}

template<class T>
bool block_queue<T>::get_back(T& value)
{
	mutex.lock();
	if(size == 0)
	{
		mutex.unlock();
		return false;
	}
	else
	{
		value = array[back];
		mutex.unlock();
		return true;
	}
}

template<class T>
int block_queue<T>::get_size()
{
	int tmp = 0;

	mutex.lock();
	tmp = size;
	mutex.unlock();
	
	return tmp;
}

template<class T>
int block_queue<T>::get_max_size()
{
	int tmp = 0;

	mutex.lock();
	tmp = max_size;
	mutex.unlock();
	
	return tmp;
}

template<class T>
bool block_queue<T>::push(const T& item)
{
	mutex.lock();

	if(size >= max_size)
	{
		cond.wake_up_all();
		mutex.unlock();
		return false;
	}

	//将新增数据放在循环数组的对应位置
	back = (back + 1) % max_size;
	array[back] = item;
	++size;

	cond.wake_up_all();
	mutex.unlock();

	return true;
}

template<class T>
bool block_queue<T>::pop(T& item)
{
	mutex.lock();

	//多个消费者的时候，这里要是用while而不是if
	while(size <= 0)
	{
		//当重新抢到互斥锁，wait返回为true
		if(!cond.wait(mutex.get_lock()))
		{
			mutex.unlock();
			return false;
		}
	}

	front = (front + 1) % max_size;
	item = array[front];
	--size;
	mutex.unlock();
	return true;
}

template<class T>
bool block_queue<T>::pop(T& item,int ms_timeout)
{
	struct timespec t = {0, 0};
	struct timeval now = {0, 0};
	gettimeofday(&now,NULL);

	mutex.lock();

	if(size <= 0)
	{
		t.tv_sec = now.tv_sec + ms_timeout /1000;
		t.tv_nsec = (ms_timeout % 1000) * 1000;
		if(!cond.time_wait(mutex.get_lock(),t))
		{
			mutex.unlock();
			return false;
		}
	}

	if(m_size <= 0)
	{
		mutex.unlock();
		return false;
	}

	front = (front + 1) % max_size;
	item = array[front];
	--size;
	mutex.unlock();
	return true;
}