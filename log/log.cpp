#include "log.h"

Log* Log::get_instance()
{
	static Log instance;
	return &instance;
}

// 异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char *file_name, int m_log_buf_size = 8192, int m_split_lines = 5000000, int max_queue_size = 0)
{
	//如果设置了max_queue_size,则设置为异步
	if(max_queue_size >= 1)
	{
		//设置写入方式flag
		is_async = true;

		//创建并设置阻塞队列长度
		log_queue = new block_queue<std::string>(max_queue_size);

		pthread_t tid;

		//flush_log_thread为回调函数,这里表示创建线程异步写日志
		pthread_create(&tid,NULL,flush_log_thread,NULL);
	}

	//输出内容的长度
	log_buf_size = m_log_buf_size;
	w_buf = new char[log_buf_size];
	memset(w_buf,'\0',sizeof(w_buf));

	//日志的最大行数
	split_lines = m_split_lines;

	time_t t = time(NULL);
	struct tm* sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm; 

	//从后往前找到第一个/的位置
	const char* p = strrchr(file_name,'/');
	char log_full_name[256] = {0};

	//相当于自定义日志名
	//若输入的文件名没有/，则直接将时间+文件名作为日志名
	if(p == NULL)
	{
		snprintf(log_full_name,255,"%d_%02d_%02d_%s",my_tm.tm_year + 1900,my_tm.tm_mon + 1,my_tm.tm_mday,file_name);
	}
	else
	{
		//将/的位置向后移动一个位置，然后复制到logname中
		//p - file_name + 1是文件所在路径文件夹的长度
		//dirname相当于./
		strcpy(log_name,p+1);
		strncpy(dir_name,file_name,p - file_name + 1);

		//后面的参数跟format有关
		snprintf(log_full_name,255,"%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
	}

	today = my_tm.tm_mday;

	fp = fopen(log_full_name,"a");
	if(fp == NULL)
	{
		return false;
	}
	return true;
}

// 异步写日志公有方法，调用私有方法async_write_log
static void *flush_log_thread(void *arg);

// 将输出内容按照标准格式整理
void Log::write_log(int level, const char *format, ...)
{
	struct timeval now = {0,0};
	gettimeofday(&now,NULL);
	time_t t = now.tv_sec;
	struct tm* sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;
	char s[16] = {0};

	//日志分级
	if(level == 0)
	{
		strcpy(s,"[debug]:");
	}
	else if(level == 1)
	{
		strcpy(s,"[info]:");
	}
	else if(level == 2)
	{
		strcpy(s,"[warn]:");
	}
	else if(level == 3)
	{
		strcpy(s,"[error]:");
	}
	else
	{
		strcpy(s,"[info]:");
	}

	mutex.lock();

	//更新现有行数
	++count;

	//日志不是今天或写入的日志行数是最大行的倍数
	//split_lines为最大行数
	if(today != my_tm.tm_mday || count % split_lines == 0)
	{
		char new_log[256] = {0};
		fflush(fp);
		fclose(fp);
		char tail[16] = {0};

		//格式化日志名中的时间部分
		snprintf(tail,16,"%d_%02d_%02d_",my_tm.tm_year + 1900,my_tm.tm_mon + 1,my_tm.tm_mday);

		//如果是时间不是今天,则创建今天的日志，更新m_today和m_count
		if(today != my_tm.tm_mday)
		{
			snprintf(new_log,255,"%s%s%s",dir_name,tail,log_name);
			today = my_tm.tm_mday;
			count = 0;
		}
		else
		{
			//超过了最大行，在之前的日志名基础上加后缀, count/split_lines
			snprintf(new_log,255,"%s%s%s.%lld",dir_name,tail,log_name,count/split_lines);
		}
		fp = fopen(new_log,"a");
	}

	mutex.unlock();

	va_list valst;
	//将传入的format参数赋值给valst，便于格式化输出
	va_start(valst,format);

	std::string log_str;
	mutex.lock();

	//写入内容格式：时间 + 内容
	//时间格式化，snprintf成功返回写字符的总数，其中不包括结尾的null字符
	int n = snprintf(w_buf,48,"%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
						my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
						my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

	//内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
	int m = vsnprintf(w_buf + n,log_buf_size - 1,format,valst);
	w_buf[n+m] = '\n';
	w_buf[n+m+1] = '\0';

	log_str = w_buf;

	mutex.unlock();

	//is_async为true表示异步，默认同步
	if(is_async && !log_queue->is_full())
	{
		log_queue->push(log_str);
	}
	else
	{
		mutex.lock();
		fputs(log_str.c_str(),fp);
		mutex.unlock();
	}
	va_end(valst);
}

// 强制刷新缓冲区
void Log::flush()
{
	mutex.lock();
	//强制刷新写入流缓冲区
	fflush(fp);
	mutex.unlock();
}

Log::Log()
{
	count = 0;
	is_async = false;
}

Log::~Log()
{
	if(fp != NULL)
	{
		fclose(fp);
	}
}

// 异步写日志方法
void* Log::async_write_log()
{
	std::string single_log;
	//从阻塞队列取出一个日志string，写入文件
	while(log_queue->pop(single_log))
	{
		mutex.lock();
		fputs(single_log.c_str(),fp);
		mutex.unlock();
	}
}

void* Log::flush_log_thread(void* arg)
{
	Log::get_instance()->async_write_log();
}
