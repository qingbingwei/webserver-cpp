#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <list>
#include <string>
#include <mysql/mysql.h>
#include <iostream>

#include "../lock_sem_cond/lock_sem_cond.h"

class connection_pool
{
public:
	MYSQL* Get_Connection(); //获取数据库连接
	bool Release_Connection(MYSQL* conn); //释放连接
	int Get_Free_Connection();//获取连接数
	void Destroy_Pool();//销毁所有连接

	//局部静态变量的单例模式
	static connection_pool* Get_Instance();

	void init(std::string m_url,std::string user,std::string Password,std::string DBname,int m_port,unsigned int MaxConn);

private:
	connection_pool();
	~connection_pool();

	int Max_Conn; //最大连接数
	int Cur_Conn; //当前连接数
	int Free_Conn; //空闲连接数
	Lock mutex;
	std::list<MYSQL*> conn_List; //连接池
	Sem reserve;

public:
	std::string url; //主机地址
	std::string port; //数据库端口号
	std::string User;//登录数据库用户名
	std::string PassWord;//登录数据库密码
	std::string DataBase_Name;//使用数据库名
	bool close_log; //日志开关

};

class connection_RAII
{
public:
	//二级指针对MYSQL *con修改
	connection_RAII(MYSQL** conn,connection_pool* connPool);
	~connection_RAII();

private:
	MYSQL* conn_RAII;
	connection_pool* Pool_RAII;

};

#endif 