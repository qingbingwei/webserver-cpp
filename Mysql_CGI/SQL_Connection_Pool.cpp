#include "SQL_Connection_Pool.h"

connection_pool::connection_pool()
{
	Cur_Conn = 0;
	Free_Conn = 0;
}

connection_pool::~connection_pool()
{
	Destroy_Pool();
}

connection_pool* connection_pool::Get_Instance()
{
	static connection_pool conn_Pool;
	return &conn_Pool;
}


MYSQL* connection_pool::Get_Connection()
{
	MYSQL* conn = NULL;

	if(conn_List.size() == 0)
	{
		return NULL;
	}

	//取出连接，信号量减1，为0则等待
	reserve.P();

	mutex.lock();

	conn = conn_List.front();
	conn_List.pop_front();

	//没用到
	--Free_Conn;
	++Cur_Conn;

	mutex.unlock();

	return conn;
}

bool connection_pool::Release_Connection(MYSQL* conn)
{
	if(conn == NULL)
	{
		return false;
	}

	mutex.lock();
	
	conn_List.push_back(conn);
	++Free_Conn;
	--Cur_Conn;

	mutex.unlock();

	//释放连接，信号量加1
	reserve.V();
	return true;
}

int connection_pool::Get_Free_Connection()
{
	return this->Free_Conn;
}

void connection_pool::Destroy_Pool()
{
	mutex.lock();

	if(conn_List.size() > 0)
	{
		//通过迭代器遍历，关闭数据库连接
		std::list<MYSQL*>::iterator it;
		for(it = conn_List.begin();it != conn_List.end();++it)
		{
			MYSQL* conn = *it;
			mysql_close(conn);
		}

		Cur_Conn = 0;
		Free_Conn = 0;

		//清空list
		conn_List.clear();

		mutex.unlock();
	}

	mutex.unlock();
}

void connection_pool::init(std::string m_url,std::string user,std::string Password,std::string DBname,int m_port,unsigned int MaxConn)
{
	//初始化数据库信息
	this->url = m_url;
	this->port = m_port;
	this->User = user;
	this->PassWord = Password;
	this->DataBase_Name = DBname;

	//创建MaxConn条数据库连接
	for(int i = 0;i < Max_Conn;++i)
	{
		MYSQL* conn = NULL;
		conn = mysql_init(conn);

		if(conn == NULL)
		{
			std::cout << "Error:" << mysql_errno(conn);
			exit(1);
		}
		conn = mysql_real_connect(conn,url.c_str(),User.c_str(),PassWord.c_str(),DBname.c_str(),m_port,NULL,0);

		if(conn == NULL)
		{
			std::cout << "Error:" << mysql_errno(conn);
			exit(1);
		}

		//更新连接池和空闲连接数量
		conn_List.push_back(conn);
		++Free_Conn;
	}
	//将信号量初始化为最大连接次数
	reserve = Sem(Free_Conn);
	this->Max_Conn = Free_Conn;
}

connection_RAII::connection_RAII(MYSQL** conn,connection_pool* connPool)
{
	*conn = connPool->Get_Connection();

	conn_RAII = *conn;
	Pool_RAII = connPool;
}

connection_RAII::~connection_RAII()
{
	Pool_RAII->Release_Connection(conn_RAII);
}