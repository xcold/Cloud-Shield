//C语言实现日志管理系统
//分别管理三个不同的模块

#include "./../PublicAPI.h"	//公用API

//==================================================
//静态全局变量 作用域：本文件
//==================================================

static pthread_mutex_t Log_Agent_pmt; //Agent模块日志文件互斥锁
static pthread_mutex_t Log_Daemon_pmt; //守护进程模块日志文件互斥锁
static pthread_mutex_t Log_AgentDL_pmt; //更新模块日志文件互斥锁

static FILE *Log_Agent_fp;
static FILE *Log_Daemon_fp;
static FILE *Log_AgentDL_fp;

//==================================================
//内部声明函数 作用域：本文件
//==================================================
static int Log_Is_File_Exist(const char *file_path);

static int Log_Is_Dir_Exist(const char *dir_path);

static void Log_GetNowDate(char* NowTime);

static void Log_Agent_Add(char *msg);

static void Log_Daemon_Add(char *msg);

static void Log_AgentDL_Add(char *msg);

static void GetNowTime(char* NowTime);

//获取当前的具体时间  2016-12-21 16:49:51
void GetNowTime(char* NowTime)
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(NowTime, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", 1900+timenow->tm_year,1+timenow->tm_mon,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
}

//初始化日志数据
void Log_Init()
{
	Log_Agent_fp = NULL;
	Log_Daemon_fp = NULL;
	Log_AgentDL_fp = NULL;

	pthread_mutex_init(&Log_Agent_pmt, NULL);
	pthread_mutex_init(&Log_Daemon_pmt, NULL);
	pthread_mutex_init(&Log_AgentDL_pmt, NULL);
}

//判断文件是否存在 1表示存在 0表示不存在
int Log_Is_File_Exist(const char *file_path)
{
	if(file_path == NULL) return 0;
	if(access(file_path, F_OK) == 0) return 1;
	return 0;
}

//判断目录是否存在 1表示存在 0表示不存在
int Log_Is_Dir_Exist(const char *dir_path)
{
	if(dir_path == NULL) return 0;
	if(opendir(dir_path) == NULL) return 0;
	return 1;
}

//获取当前的日期  2016-12-21
void Log_GetNowDate(char* NowTime)
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(NowTime, "%.4d-%.2d-%.2d", 1900+timenow->tm_year,1+timenow->tm_mon,timenow->tm_mday);
}

void Log_Agent_Add(char *msg)
{
	pthread_mutex_lock(&Log_Agent_pmt);

	char DateString[105] = {0};
	char LogPath[105] = {0};
	//根据name判断是那个目录
	sprintf(LogPath, "./Log_Agent/");
	Log_GetNowDate(DateString);
	strcat(LogPath, DateString);
	//判断当天的日志文件是否存在
	int res = Log_Is_File_Exist(LogPath);

	if(Log_Agent_fp == NULL)
	{
		Log_Agent_fp = fopen(LogPath, "a+");
		printf("logpath:%s\n", LogPath);
		//设置缓存区大小
		//setbuf();
	}
	else
	{
		if(res == 1) //文件已存在
		{

		}
		else //当天文件不存在
		{
			fclose(Log_Agent_fp);//关闭头一天的文件
			Log_Agent_fp = NULL;
			Log_Agent_fp = fopen(LogPath, "a+");//新建今天的
		}

	}
	char TimeString[105] = {0};
	GetNowTime(TimeString);
	//构造日志格式  时间：xxxx-xx-xx\t内容：buf
	char buf[4096] = {0};
	sprintf(buf, "时间：%s\t内容：%s\n", TimeString, msg);
	fwrite(buf, strlen(buf), 1, Log_Agent_fp);

	fflush(Log_Agent_fp); //刷新缓存区
	pthread_mutex_unlock(&Log_Agent_pmt);
}

void Log_Daemon_Add(char *msg)
{
	pthread_mutex_lock(&Log_Daemon_pmt);

	char DateString[105] = {0};
	char LogPath[105] = {0};
	//根据name判断是那个目录
	sprintf(LogPath, "./Log_Daemon/");
	Log_GetNowDate(DateString);
	strcat(LogPath, DateString);
	//判断当天的日志文件是否存在
	int res = Log_Is_File_Exist(LogPath);

	if(Log_Daemon_fp == NULL)
	{
		Log_Daemon_fp = fopen(LogPath, "a+");
		//设置缓存区大小
		//setbuf();
	}
	else
	{
		if(res == 1) //文件已存在
		{

		}
		else //当天文件不存在
		{
			fclose(Log_Daemon_fp);//关闭头一天的文件
			Log_Daemon_fp = NULL;
			Log_Daemon_fp = fopen(LogPath, "a+");//新建今天的
		}

	}
	char TimeString[105] = {0};
	GetNowTime(TimeString);
	//构造日志格式  时间：xxxx-xx-xx\t内容：buf
	char buf[4096] = {0};
	sprintf(buf, "时间：%s\t内容：%s\n", TimeString, msg);
	fwrite(buf, strlen(buf), 1, Log_Daemon_fp);

	fflush(Log_Daemon_fp); //刷新缓存区
	pthread_mutex_unlock(&Log_Daemon_pmt);
}

void Log_AgentDL_Add(char *msg)
{
	pthread_mutex_lock(&Log_AgentDL_pmt);

	char DateString[105] = {0};
	char LogPath[105] = {0};
	//根据name判断是那个目录
	sprintf(LogPath, "./Log_AgentDL/");
	Log_GetNowDate(DateString);
	strcat(LogPath, DateString);
	//判断当天的日志文件是否存在
	int res = Log_Is_File_Exist(LogPath);

	if(Log_AgentDL_fp == NULL)
	{
		Log_AgentDL_fp = fopen(LogPath, "a+");
		//设置缓存区大小
		//setbuf();
	}
	else
	{
		if(res == 1) //文件已存在
		{

		}
		else //当天文件不存在
		{
			fclose(Log_AgentDL_fp);//关闭头一天的文件
			Log_AgentDL_fp = NULL;
			Log_AgentDL_fp = fopen(LogPath, "a+");//新建今天的
		}

	}
	char TimeString[105] = {0};
	GetNowTime(TimeString);
	//构造日志格式  时间：xxxx-xx-xx\t内容：buf
	char buf[4096] = {0};
	sprintf(buf, "时间：%s\t内容：%s\n", TimeString, msg);
	fwrite(buf, strlen(buf), 1, Log_AgentDL_fp);

	fflush(Log_AgentDL_fp); //刷新缓存区
	pthread_mutex_unlock(&Log_AgentDL_pmt);
}

//
//name表示是哪个模块的日志系统
//msg需要记录的日志消息
void Log_Add(char* name, char *msg)
{
	printf("name:%s msg:%s\n", name, msg);
	//为防止使用同一互斥锁会影响执行效率、根据不同模块分别调用
	if(strcmp(name, "Log_Agent") == 0)
	{
		Log_Agent_Add(msg);
	}
	else if(strcmp(name, "Log_Daemon") == 0)
	{
		Log_Daemon_Add(msg);
	}
	else if(strcmp(name, "Log_AgentDL") == 0)
	{
		Log_AgentDL_Add(msg);
	}
	//以后扩展别的模块日志系统
	return;
}
