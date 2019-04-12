Agent//C语言实现Agent部分
//主要包括3个模块、脚本插件管理系统、安全防护管理系统、Agent自检管理系统
//2个通讯模块、与大数据服务器使用HTTP通信、与云盾服务端使用TCP通信



//包含头文件
#include "PublicAPI.h"	//公用API
#include "ThreadPool/ThreadPool.h"
#include "Plugin/Plugin.h"
#include "Log/Log.h"
//#include "Security/Security.h"
#include "Tcp/Tcp.h"
#include "AES/AES.h"
#include "AES/MKAES.h"
#include "Http/AgentHttp.h"

//==================================================
//数据定义	作用域：本文件
//==================================================
#define MAXN 1005
#define THREAD_NUM 5

//==================================================
//引用全局变量	作用域：本文件
//==================================================
extern BYTE UserKey[AES_USER_KEY_LEN]; //AES加密密钥


//==================================================
//全局变量	作用域：整个程序
//==================================================
//Agent日志模块
void Agent_Log(char *buf)
{
	Log_Add("Log_Agent", buf);
}

//system升级版、避免系统信号突然改变的坑
int my_system(char *cmd, char *result)
{
	FILE *pp = popen(cmd, "r"); //建立管道
	if (pp == NULL) {
        char log[BUFSIZE] = {0};
		sprintf(log, "popen函数执行出错:%s", strerror(errno));
		Agent_Log(log);
		return -1;
    }
	memset(result, 0, sizeof(&result));
	char tmp[BUFSIZE] = {0};
	int len = 0;
	int len_tmp = 0;
    while(fgets(tmp, sizeof(tmp), pp) != NULL)
	{
		len_tmp = strlen(tmp);
		len = len + len_tmp;
		if(len > 3500)
		{
			Agent_Log("运行结果输出太长，无法展示完");
			break;
		}
		strcat(result, tmp);
	}

	int res = 0;
    if((res = pclose(pp)) == -1)//关闭管道
	{
		Agent_Log("popen关闭函数出错!");
		return -1;
	}
	return res;
}


//==================================================
//静态全局变量	作用域：本文件
//==================================================



//==================================================
//内部声明函数	作用域：本文件
//==================================================
static void Agent_RunAgain();

static void Agent_Password_Decrypt(char *buf);

static void Agent_Judge_Password();

static void Agent_Init();






//判断Agent是否已启动
void Agent_RunAgain()
{
	char result[4096] = {0};
	int res = my_system("pidof Agent", result);

	if(res == -1) exit(0);

	int len = strlen(result);
	int i;
	int num = 0;
	int count = 0;
	for(i = 0; i < len; i++)
	{
		if(result[i] >= '0' && result[i] <= '9')
		{
			num = num * 10 + result[i] - '0';
		}
		else
		{
			if(num != 0) count++;
			num = 0;
		}
	}
	if(count > 1)
	{
		Agent_Log("Agent重复启动!");
		exit(0);
	}
}

//解密Password
void Agent_Password_Decrypt(char *buf)
{
	int len = strlen(buf);
	int i;
	for(i = 0; i < len; i++)
	{
		buf[i] = buf[i] + 23;
		buf[i] = buf[i] ^ 0x5678;
		buf[i] = buf[i] - 127;
		buf[i] = buf[i] ^ 0x1234;
	}
}

//获取配置文件下的通信秘钥
//解密判断通信秘钥是否符合标准
//dy@xxxxxxxxxxxxxxxx
void Agent_Judge_Password()
{
	char Agent_Tcp_Password[BUFSIZE] = {0}; //通信密钥
	FILE *fp;
	fp = fopen("./Passwd.key", "r");
	if(fp == NULL)
	{
		//写入日志
		Agent_Log("无法打开Passwd.key文件");
		exit(EXIT_FAILURE);
	}
	char pwd[105] = {0};
	fgets(pwd, sizeof(pwd), fp);
	int len = strlen(pwd);
	if(len < 19 || len > 20) exit(EXIT_SUCCESS); //密钥长度不对 换行符号有误差影响
	//Agent_Password_Decrypt(pwd); //解密暂时不用
	if(strstr(pwd, "dy@") == NULL) //秘钥格式错误
	{
		//写入日志
		Agent_Log("密钥格式错误");
		exit(EXIT_SUCCESS);
	}
	if(len >= sizeof(Agent_Tcp_Password)) //
	{
		Agent_Log("密钥格式错误");
		exit(EXIT_SUCCESS);
	}

	int i, j = 0;
	memset(Agent_Tcp_Password, 0, sizeof(Agent_Tcp_Password));
	for(i = 3; i < len; i++)
	{
		Agent_Tcp_Password[j++] = pwd[i];
	}
	//将字符转化为16进制密钥
	ChangeUserKey(Agent_Tcp_Password);
	printf("pwd:%s\n", Agent_Tcp_Password);
}



//Agent启动初始化过程
void Agent_Init()
{
	//初始化日志系统
	Log_Init();

	//判断是否重复启动Agent
	Agent_RunAgain();

	//判断Agent是否有通信秘钥
	Agent_Judge_Password();


	int ret = 0;

	//初始化服务端交互系统
	Tcp_Init();

	Http_Init();


	//初始化插件管理系统
	ret = Plugin_Init();
	if(ret != 0)
	{
		Agent_Log("插件管理系统初始化失败");
		exit(EXIT_FAILURE);
	}

	/*
	//初始化安全管理系统
	ret = Security_Init();
	if(ret != 0)
	{
		Agent_Log("安全管理系统初始化失败");
		exit(EXIT_FAILURE);
	}
	*/

	return;
}



//主线程 进程资源管理系统&程序错误处理系统
int main()
{

	//初始化各类数据、为程序的运行做资源初始化
	Agent_Init();

	pthread_t thread[THREAD_NUM];
	int ret[THREAD_NUM];
	memset(ret, 0, sizeof(ret));


	//服务端交互系统的线程
	ret[0] = pthread_create(&thread[0], NULL, (void *)Server_Interactive, NULL);
	if(ret[0] != 0)//创建线程失败
	{
		//写入日志
		Agent_Log("Server_Interactive Thread create failed!\n");
		exit(EXIT_FAILURE);//退出程序
	}


	//插件管理系统的线程
	ret[1] = pthread_create(&thread[1], NULL, (void *)Plugin_Manage, NULL);
	if(ret[1] != 0)//创建线程失败
	{
		//写入日志
		Agent_Log("Plugin_Manage Thread create failed!\n");
		exit(EXIT_FAILURE);//退出程序
	}


	/*
	//安全管理系统的线程
	ret[2] = pthread_create(&thread[2], NULL, (void *)Security_Manage, NULL);
	if(ret[2] != 0)//创建线程失败
	{
		//写入日志
		Agent_Log("Security_Manage Thread create failed!\n");
		exit(EXIT_FAILURE);//退出程序
	}
	*/
	//Agent管理系统
	AgentManage_Running();

	return 0;
}
