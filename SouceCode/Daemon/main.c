// C语言实现守护进程
// 通过HTTP/POST轮询agent的状态来开启或关闭agent
// 如果Agent自动退出或崩溃根据服务端获取的状态来重启Agent
// Daemon程序是后台进程、服务器开机自动启动、防止服务器意外宕机重启的情况

//C语言库函数
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

//POSIX标准
#include <unistd.h>

//linux系统函数
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/vfs.h>
#include <sys/select.h>
#include <sys/param.h>

#include "cJSON/cJSON.h" //使用cjson解析数据
#include "Log/Log.h"

#define EXIT_SUCCESS 0  //正常退出exit(0)
#define EXIT_FAILURE  1  //异常退出exit(1)

#define RUN 1
#define STOP 0

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int thread_status = RUN;

//#define IPSTR "10.17.17.98" //云盾服务端IP
//#define IPSTR "10.32.64.207" //外网云盾服务端IP
//#define IPSTR "10.17.65.112" //内网云盾服务端IP
//#define IPSTR "10.17.18.235" //云盾服务端IP
//#define PORT 38080  //云盾服务端端口
#define BUFSIZE 4096 //单次发送的数据最大长度

//#define char unsigned char

//----------------全局变量-----------------------start
static pthread_mutex_t Agent_Status_pmt; //Agent状态标记锁

int Agent_Status;

FILE *fp; //这个命名考虑修改一下

static pthread_mutex_t File_Open_pmt; //日志文件锁

//云盾服务端IP端口列表
struct YunDunServerIP
{
	char ipstr[32]; //ip
	int port; //端口
	int level; //连接优先级
}YunDunServerIP_List[1005]; //云盾服务端IP列表

int PORT = 0;
char IPSTR[32] = {0};
int YunDunServerIP_MaxNum = 0;

int agent_pid = 0;

pid_t pid;
pid_t child_id;
//----------------全局变量-----------------------end

void Daemon_Init();

int Is_File_Exist(const char *file_path);

int Is_Dir_Exist(const char *dir_path);

void GetNowDate(char* NowTime);

void GetNowTime(char* NowTime);

void Daemon_Log(char *buf);

void Create_Daemon();

void Daemon_Register();

void Daemon_RunStatus();

int Base64Decode( char *OrgString, char *Base64String, int Base64StringLen, int bForceDecode );

char GetBase64Value(char ch); //得到编码值

int Base64Encode( char *OrgString, char *Base64String, int OrgStringLen );

int Judge_Agent_Status();

void Daemon_Destroy();

void SendMessage(char* sendmsg);



void thread_resume()
{
    if (thread_status == STOP)
    {
        pthread_mutex_lock(&mut);
        thread_status = RUN;
        pthread_cond_signal(&cond);
        printf("pthread run!\n");
        pthread_mutex_unlock(&mut);
    }
    else
    {
        printf("pthread run already\n");
    }
}

void thread_pause()
{
    if (thread_status == RUN)
    {
        pthread_mutex_lock(&mut);
        thread_status = STOP;
        printf("thread stop!\n");
        pthread_mutex_unlock(&mut);
    }
    else
    {
        printf("pthread pause already\n");
    }
}


//守护进程通信初始化
void Socket_Init()
{
    memset(&YunDunServerIP_List, 0, sizeof(YunDunServerIP_List));
    //从配置文件中读取云盾服务端IP和端口
    FILE *fp;
    fp = fopen("./ServerIP/YunDunServerIP_Http.ini", "r");
    if(fp == NULL)
    {
        Daemon_Log("打开YunDunServerIP文件失败");
        exit(EXIT_FAILURE);
    }
    char buf[BUFSIZE] = {0};
    while(fgets(buf, sizeof(buf), fp) != NULL);
    int len = strlen(buf);
    if(len < 10)
    {
        Daemon_Log("YunDunServerIP配置文件长度错误");
        exit(EXIT_FAILURE);
    }

    int i, flag = 0;
    int port = 0;
    char ipstr[32] = {0};
    for(i = 0; i < len; i++)
    {
        if(buf[i] == '\n')//换行重置
        {
            flag = 0;
            port = 0;
            memset(ipstr, 0, sizeof(ipstr));
        }
        if(buf[i] == ':')
        {
            flag = 1;
        }
        if(buf[i] == ' ')
        {
            flag = 2;
        }
        if(flag == 0 && buf[i] != ':')
        {
            ipstr[i] = buf[i];
        }
        if(flag == 1 && buf[i] != ':')
        {
            port = port * 10 + buf[i] - '0';

        }
        if(flag == 2 && buf[i] != ' ')
        {
            int ip_len = strlen(ipstr);
            if(ip_len >= 24)
            {
                Daemon_Log("YunDunServerIP配置文件IP错误");
                exit(EXIT_FAILURE);
            }
            strcpy(YunDunServerIP_List[YunDunServerIP_MaxNum].ipstr, ipstr);

            YunDunServerIP_List[YunDunServerIP_MaxNum].port = port;
            if(YunDunServerIP_List[YunDunServerIP_MaxNum].port <= 1 || YunDunServerIP_List[YunDunServerIP_MaxNum].port >= 65535)
            {
                Daemon_Log("YunDunServerIP配置文件端口错误");
                exit(EXIT_FAILURE);
            }

            YunDunServerIP_List[YunDunServerIP_MaxNum].level = buf[i] - '0';
            if(YunDunServerIP_List[YunDunServerIP_MaxNum].level < 1 || YunDunServerIP_List[YunDunServerIP_MaxNum].level > 9)
            {
                Daemon_Log("YunDunServerIP配置文件优先级错误");
                exit(EXIT_FAILURE);
            }
            YunDunServerIP_MaxNum++;
        }
    }
    if(YunDunServerIP_MaxNum <= 0)
    {
        Daemon_Log("BigDataServerIP配置文件未找到IP列表");
        exit(EXIT_SUCCESS);
    }
	printf("服务端IP：%s:%d %d\n", YunDunServerIP_List[0].ipstr, YunDunServerIP_List[0].port, YunDunServerIP_List[0].level);
	char log[1024] = {0};
	sprintf(log, "服务端IP：%s:%d %d\n", YunDunServerIP_List[0].ipstr, YunDunServerIP_List[0].port, YunDunServerIP_List[0].level);
	Daemon_Log(log);

	//printf("%s:%d\n", IPSTR, PORT);
}

void Daemon_Http_Test()
{

	int connect_id = 0;
	int Repeat_Connect = 0;
	//尝试链接
	while(1)
	{
		connect_id = connect_id % YunDunServerIP_MaxNum;
		PORT = YunDunServerIP_List[connect_id].port;
		strcpy(IPSTR, YunDunServerIP_List[connect_id].ipstr);

		//connect
	    int sockfd;
	    struct sockaddr_in servaddr;
	    socklen_t len;
		//fd_set t_set1;

	    int ret = 0; //定义返回值
	    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--socket error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字

			connect_id++;
	        continue; //网络断开再次重新链接
	    }
	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(PORT);
	    if (inet_pton(AF_INET, IPSTR, &servaddr.sin_addr) <= 0 ){
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--inet_pton error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字

			connect_id++;
	        continue; //网络断开再次重新链接
	    }
		//自动尝试3次连接 每次间隔10S
	    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			char log[1024] = {0};
			sprintf(log, "连接到服务器失败,即将重新链接--connect error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字

			connect_id++;
	        continue; //网络断开再次重新链接
	    }
	    Daemon_Log("与服务端建立了连接...\n");
		close(sockfd); //关闭套接字
		//sleep(3);
		break;
	}
}

//进程初始化
void Daemon_Init()
{
	Agent_Status = -1;
	fp = NULL;

	Socket_Init();
	//初始化锁
	pthread_mutex_init(&Agent_Status_pmt, NULL);
	pthread_mutex_init(&File_Open_pmt, NULL);
}

//判断文件是否存在 1表示存在 0表示不存在
int Is_File_Exist(const char *file_path)
{
	if(file_path == NULL) return 0;
	if(access(file_path, F_OK) == 0) return 1;
	return 0;
}

//判断目录是否存在 1表示存在 0表示不存在
int Is_Dir_Exist(const char *dir_path)
{
	if(dir_path == NULL) return 0;
	if(opendir(dir_path) == NULL) return 0;
	return 1;
}

//获取当前的日期  2016-12-21
void GetNowDate(char* NowTime)
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(NowTime, "%.4d-%.2d-%.2d", 1900+timenow->tm_year,1+timenow->tm_mon,timenow->tm_mday);
}

//获取当前的具体时间  2016-12-21 16:49:51
void GetNowTime(char* NowTime)
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(NowTime, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", 1900+timenow->tm_year,1+timenow->tm_mon,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
}

//日志系统、加上时间
//文件格式：xxxx-xx-xx.txt  年月日
void Daemon_Log(char *buf)
{
	printf("Daemon_Log:%s\n", buf);
	Log_Add("Log_Daemon", buf);
	/*
	pthread_mutex_lock(&File_Open_pmt);


	char DateString[105] = {0};
	GetNowDate(DateString);
	//判断当天的日志文件是否存在
	int res = Is_File_Exist(DateString);

	if(fp == NULL)
	{
		fp = fopen(DateString, "a+");
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
			fclose(fp);//关闭头一天的文件
			fp = NULL;
			fp = fopen(DateString, "a+");//新建今天的
		}

	}
	char TimeString[105] = {0};
	GetNowTime(TimeString);
	//构造日志格式  时间：xxxx-xx-xx\t内容：buf
	char msg[1024] = {0};
	sprintf(msg, "时间：%s  内容：%s\n", TimeString, buf);
	fwrite(msg, strlen(msg), 1, fp);

	fflush(fp); //刷新缓存区
	pthread_mutex_unlock(&File_Open_pmt);
	*/
}

//创建后台进程
void Create_Daemon()
{
	pid_t pid = fork(); // 返回两个值、-1表示错误、0表示子进程、其他表示父进程
	struct sigaction sa;

	if(pid == -1)//fork失败
	{
		Daemon_Log("fork error!\n");
		exit(EXIT_FAILURE);
	}
	else if(pid != 0)//父进程
	{
		exit(EXIT_SUCCESS);
	}

	if(setsid() == -1)
	{
		Daemon_Log("setsid failed!\n");
		exit(EXIT_FAILURE);
	}

	umask(0);
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGCHLD, &sa, NULL) < 0)
	{
		return;
	}

	pid = fork(); //再次fork
	if(pid < 0)
	{
		Daemon_Log("fork error!\n");
		return;
	}
	else if(pid != 0)
	{
		exit(EXIT_SUCCESS);
	}
	//second children
	if(chdir("/") < 0)
	{
		Daemon_Log("child dir error!\n");
		return;
	}
	int i;
	for(i = 0; i < NOFILE; i++)
	{
		close(i);
	}
	int fd = open("/dev/null", O_RDWR);
	dup2(fd, 1);
	dup2(fd, 2);
}

void SendMessage(char* sendmsg)
{
	int Repeat_Connect = 0;
	int Repeat_Send = 0;
	char sendbuf[BUFSIZE];
	char recvbuf[BUFSIZE];

	while(1)
	{
		//connect
	    int sockfd;
	    struct sockaddr_in servaddr;
	    socklen_t len;
		//fd_set t_set1;

	    int ret = 0; //定义返回值

	    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--socket error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(PORT);
	    if (inet_pton(AF_INET, IPSTR, &servaddr.sin_addr) <= 0 ){
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--inet_pton error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

		//自动尝试3次连接 每次间隔10S
	    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			char log[1024] = {0};
			sprintf(log, "连接到服务器失败,即将重新链接--connect error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

	    Daemon_Log("与服务端建立了连接...\n");

		Daemon_Log(sendmsg);
	    char Encodemsg[4096] = {0};
		Base64Encode(sendmsg, Encodemsg, strlen(sendmsg));//数据加密、只加密body部分
		len = strlen(Encodemsg);
		//strcat(sendbuf, Encodemsg); //错误、重复发送

		//清空
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));

		//http消息头构造
		sprintf(sendbuf, "POST / HTTP/1.1\n");
		strcat(sendbuf, "Host: www.duoyi.com\n");
		strcat(sendbuf, "Content-Type: application/json\n");

		char content_len[105] = {0};
		sprintf(content_len, "Content-Length: %d", len);
		strcat(sendbuf, content_len);
		strcat(sendbuf, "\n\n"); //间隔一行表示下面是内容

		strcat(sendbuf, Encodemsg);

		//发送http请求
		ret = send(sockfd, sendbuf, strlen(sendbuf), 0);//发送的时候用strlen
		if(ret < 0)//send数据错误重发
		{
			char log[1024] = {0};
			sprintf(log, "http send error!--已连续重发%d次\n", Repeat_Send);
			Daemon_Log(log);
			close(sockfd); //关闭套接字
			continue;
		}
		else
		{
			Repeat_Send = 0;
			char log[1024] = {0};
			sprintf(log, "消息发送成功, 共发送了%d个字节\n\n", ret);
			Daemon_Log(log);
		}

		//接收POST返回
		ret = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
		if(ret < 0)
		{
			Daemon_Log("http recv error!\n");
			close(sockfd); //关闭套接字
			continue;
		}

		char recvmsg[BUFSIZE] = {0};
		int j = 0, k = 0;
		int flag = 0;
		for(k = 4; k < sizeof(recvbuf); k++)
		{
			if(recvbuf[k] == '\0') break;
			if(flag == 1)
			{
				recvmsg[j++] = recvbuf[k];
			}
			/*
			if(recvbuf[k] == '\n' && recvbuf[k - 1] == '\r' && recvbuf[k - 2] == '\n' && recvbuf[k - 3] == '\r')
			{
				flag = 1;
			}
			 */
			if(recvbuf[k] == '\n'&& recvbuf[k - 1] == '\n')
			{
				flag = 1;
			}
		}
		recvmsg[j] = '\0';
		Daemon_Log(recvmsg);

		char Decodemsg[4096] = {0};
		Base64Decode(Decodemsg, recvmsg, strlen(recvmsg), 1);//数据解密
		Daemon_Log(Decodemsg);
		//解析接收到的返回
		cJSON *json , *json_cmd, *json_code;
		json = cJSON_Parse(Decodemsg);
    	if(!json)
    	{
			char log[1024] = {0};
			sprintf(log, "Error before: [%s]\n", Decodemsg);
        	Daemon_Log(log);
    	}
    	else
    	{
        	// 解析cmd
        	json_cmd = cJSON_GetObjectItem(json , "cmd");
        	json_code = cJSON_GetObjectItem(json, "code");
        	if( (json_cmd->type == cJSON_Number) && (json_code->type == cJSON_Number) \
        		 && (json_code->valueint == 1))
        	{
            	Daemon_Log("消息发送成功!\n");

            	free(json_cmd);
	       		json_cmd = NULL;
	       		free(json_code);
	       		json_code = NULL;

	       		//释放指针
		       	free(json);
		       	json = NULL;

				close(sockfd); //关闭套接字
            	break; //跳出循环
        	}
        	else //解析错误
        	{
        		Daemon_Log("消息发送失败!\n");
        		sleep(3);
        		continue; //重新请求
        	}
        	free(json_cmd);
       		json_cmd = NULL;
       		free(json_code);
       		json_code = NULL;
        }

        //释放指针
       	free(json);
       	json = NULL;

		close(sockfd); //关闭套接字
	}
}

//向服务端发送注册请求
void Daemon_Register()
{
	int Repeat_Connect = 0;
	int Repeat_Send = 0;
	char sendbuf[BUFSIZE];
	char recvbuf[BUFSIZE];

	while(1)
	{
		//connect
	    int sockfd;
	    struct sockaddr_in servaddr;
	    socklen_t len;
		//fd_set t_set;

	    int ret = 0; //定义返回值

	    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--socket error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        continue; //网络断开再次重新链接
	    }

	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(PORT);
	    if (inet_pton(AF_INET, IPSTR, &servaddr.sin_addr) <= 0 ){
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--inet_pton error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        continue; //网络断开再次重新链接
	    }

		//自动尝试3次连接 每次间隔10S
	    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			char log[1024] = {0};
			sprintf(log, "连接到服务器失败,即将重新链接--connect error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        continue; //网络断开再次重新链接
	    }

	    Daemon_Log("与服务端建立了连接...\n");

	    //注册请求发送
	    char sendmsg[BUFSIZE] = "{\"cmd\":9001,\"version\":\"v_0.0.1\",\"pwd\":\"SymmetricEncoder\",\"token\":\"n7sbl64qb88l0xy\"}";
		Daemon_Log(sendmsg);
	    //char *str;
	    //str = (char *)malloc(128);
	    //len = strlen(sendmsg);
	    //sprintf(str, "%d", len);

		char Encodemsg[4096] = {0};
		Base64Encode(sendmsg, Encodemsg, strlen(sendmsg));//数据加密、只加密body部分
		len = strlen(Encodemsg);
		strcat(sendbuf, Encodemsg);

		//清空
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));

		//http消息头构造
		sprintf(sendbuf, "POST / HTTP/1.1\n");
		strcat(sendbuf, "Host: www.duoyi.com\n");
		strcat(sendbuf, "Content-Type: application/json\n");

		char content_len[105] = {0};
		sprintf(content_len, "Content-Length: %d", len);
		strcat(sendbuf, content_len);
		strcat(sendbuf, "\n\n"); //间隔一行表示下面是内容

		strcat(sendbuf, Encodemsg);
		//发送http请求
		ret = send(sockfd, sendbuf, strlen(sendbuf), 0);//发送的时候用strlen
		if(ret < 0)//send数据错误重发
		{
			char log[1024] = {0};
			sprintf(log, "http send error!--已连续重发%d次\n", Repeat_Send);
			Daemon_Log(log);
			continue;
		}
		else
		{
			Repeat_Send = 0;
			char log[1024] = {0};
			sprintf(log, "消息发送成功, 共发送了%d个字节\n\n", ret);
			Daemon_Log(log);
		}

		//接收POST返回
		ret = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
		if(ret < 0)
		{
			Daemon_Log("http recv error!\n");
			continue;
		}

		Daemon_Log(recvbuf);
		char recvmsg[BUFSIZE] = {0};
		int j = 0, k = 0;
		int flag = 0;
		//printf("..... %d\n", sizeof(recvbuf));
		for(k = 4; k < sizeof(recvbuf); k++)
		{
			//printf("%c", recvbuf[k]);
			//printf(".....\n");
			if(recvbuf[k] == '\0')
			{
					//printf(".....\n");
					break;
			}
			if(flag == 1)
			{
				recvmsg[j++] = recvbuf[k];
			}
			/*
			if(recvbuf[k] == '\n' && recvbuf[k - 1] == '\r' && recvbuf[k - 2] == '\n' && recvbuf[k - 3] == '\r')
			{
				flag = 1;
			}
			 */


			if(recvbuf[k] == '\n'&& recvbuf[k - 1] == '\n')
			{
				flag = 1;
			}
		}
		//printf("%d", k);
		recvmsg[j] = '\0';
		//printf("recv %s\n", recvmsg);
		//sleep(100);

		Daemon_Log(recvmsg);
		char Decodemsg[4096] = {0};
		//printf("recvmsg:......%s\n", recvmsg);
		Base64Decode(Decodemsg, recvmsg, strlen(recvmsg), 1);//数据解密
		Daemon_Log(Decodemsg);
		//解析接收到的返回
		cJSON *json , *json_cmd, *json_code;
		json = cJSON_Parse(Decodemsg);
    	if(!json)
    	{
			char log[1024] = {0};
			sprintf(log, "Error before: [%s]\n", Decodemsg);
        	Daemon_Log(log);
    	}
    	else
    	{
        	// 解析cmd
        	json_cmd = cJSON_GetObjectItem(json , "cmd");
        	json_code = cJSON_GetObjectItem(json, "code");
        	if( json_cmd->type == cJSON_Number && json_code->type == cJSON_Number \
        		&& json_cmd->valueint == 9002 && json_code->valueint == 1)
        	{
            	Daemon_Log("注册成功!\n");

            	free(json_cmd);
	       		json_cmd = NULL;
	       		free(json_code);
	       		json_code = NULL;

	       		//释放指针
		       	free(json);
		       	json = NULL;

				close(sockfd); //关闭套接字
            	break; //跳出循环
        	}
        	else //解析错误
        	{
        		Daemon_Log("注册失败!\n");
        		//sleep(3);
        		//continue; //重新请求
        	}
        	free(json_cmd);
       		json_cmd = NULL;
       		free(json_code);
       		json_code = NULL;
        }

        //释放指针
       	free(json);
       	json = NULL;

		close(sockfd); //关闭套接字

		sleep(3);
	}
}

//HTTP轮询线程、查看运行状态
//如果连接失败自动重连
void Daemon_RunStatus()
{
	int Repeat_Connect = 0; //重连次数
	int Repeat_Send = 0; //重发次数
	char sendbuf[BUFSIZE] = {0};
	char recvbuf[BUFSIZE] = {0};
	while(1)
	{
		pthread_mutex_lock(&mut);
        while(!thread_status)
        {
            pthread_cond_wait(&cond, &mut);
        }
        pthread_mutex_unlock(&mut);

		//如果连续重连次数超过100次、就是5分钟无法连接上服务器
		//则结束轮询线程、可能线程资源和服务器上其他程序冲突导致问题
		//if(Repeat_Connect >= 100)
		//{
			//break;
		//}

		//connect
	    int sockfd;
	    struct sockaddr_in servaddr;
	    socklen_t len;
		//fd_set t_set;

	    int ret = 0;

	    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	        char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--socket error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        continue; //网络断开再次重新链接
	    }

	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(PORT);
	    if (inet_pton(AF_INET, IPSTR, &servaddr.sin_addr) <= 0 ){
			char log[1024] = {0};
			sprintf(log, "创建网络连接失败,即将重新链接--inet_pton error! 已连续重连%d次\n", Repeat_Connect);
	        Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        continue; //网络断开再次重新链接
	    }

		//自动尝试3次连接 每次间隔10S
	    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			char log[1024] = {0};
	        sprintf(log, "连接到服务器失败,即将重新链接--connect error! 已连续重连%d次\n", Repeat_Connect);
			Daemon_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        continue; //网络断开再次重新链接
	    }

	    Daemon_Log("与服务端建立了连接...\n");

	    //查询运行状态
		char sendmsg[BUFSIZE] = {0};

		sprintf(sendmsg, "{\"cmd\":9005,\"run_status\":%d,\"version\":\"v_0.0.1\",\"token\":\"n7sbl64qb88l0xy\"}", Agent_Status);
		Daemon_Log(sendmsg);
	    char Encodemsg[4096] = {0};
		Base64Encode(sendmsg, Encodemsg, strlen(sendmsg));//数据加密、只加密body部分
		len = strlen(Encodemsg);
		strcat(sendbuf, Encodemsg);

		//清空
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));

		//http消息头构造
		sprintf(sendbuf, "POST / HTTP/1.1\n");
		strcat(sendbuf, "Host: www.duoyi.com\n");
		strcat(sendbuf, "Content-Type: application/json\n");
		char content_len[105] = {0};
		sprintf(content_len, "Content-Length: %d", len);
		strcat(sendbuf, content_len);
		strcat(sendbuf, "\n\n"); //间隔一行表示下面是内容

		strcat(sendbuf, Encodemsg);

		//发送http请求
		ret = send(sockfd, sendbuf, strlen(sendbuf), 0);//发送的时候用strlen
		if(ret < 0)//send数据错误重发
		{
			char log[1024] = {0};
			sprintf(log, "http send error!--已连续重发%d次\n", Repeat_Send);
			Daemon_Log(log);
			sleep(1);
			continue;
		}
		else
		{
			Repeat_Send = 0;
			char log[1024] = {0};
			sprintf(log, "消息发送成功, 共发送了%d个字节\n\n", ret);
			Daemon_Log(log);
		}
		//接收POST返回
		ret = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
		if(ret < 0)
		{
			Daemon_Log("http recv error!\n");
			sleep(1);
			continue;
		}
		Daemon_Log(recvbuf);

		char recvmsg[BUFSIZE] = {0};
		int j = 0, k = 0;
		int flag = 0;
		for(k = 4; k < sizeof(recvbuf); k++)
		{
			if(recvbuf[k] == '\0') break;
			if(flag == 1)
			{
				recvmsg[j++] = recvbuf[k];
			}
			/*
			if(recvbuf[k] == '\n' && recvbuf[k - 1] == '\r' && recvbuf[k - 2] == '\n' && recvbuf[k - 3] == '\r')
			{
				flag = 1;
			}
			*/
			if(recvbuf[k] == '\n'&& recvbuf[k - 1] == '\n')
			{
				flag = 1;
			}
		}
		recvmsg[j] = '\0';
		Daemon_Log(recvmsg);


		char Decodemsg[4096] = {0};
		Base64Decode(Decodemsg, recvmsg, strlen(recvmsg), 1);//数据解密
		Daemon_Log(Decodemsg);
		//解析接收到的返回
		cJSON *json , *json_cmd, *json_code, *json_message, *json_status;
		cJSON *json_publish_status, *json_publish_version, *json_publish_url;
		json = cJSON_Parse(Decodemsg);
    	if(!json)
    	{
			char log[1024] = {0};
			sprintf(log, "Error before: [%s]\n", Decodemsg);
        	Daemon_Log(log);
    	}
    	else
    	{
        	// 解析
        	json_cmd = cJSON_GetObjectItem(json , "cmd");
        	json_code = cJSON_GetObjectItem(json, "code");
        	json_message = cJSON_GetObjectItem(json, "message");
        	if(json_message)
        	{
        		json_status = cJSON_GetObjectItem(json_message, "run_status");
        		json_publish_status = cJSON_GetObjectItem(json_message, "publish_status");
        	}
        	else
        	{
        		Daemon_Log("json_message error\n");
        	}
        	if((json_cmd->valueint == 9006) && (json_code->valueint == 1) && json_status != NULL)
        	{
				char log[1024] = {0};
				sprintf(log, "查询状态成功!当前状态标记为%d\n", json_status->valueint);
            	Daemon_Log(log);
            	//标记Agent的状态

				if((Agent_Status ==15 && json_status->valueint == 10) || (Agent_Status == 5 && json_status->valueint == 0) || (Agent_Status == json_status->valueint))
				{
					//服务端重复通知 不处理
				}
				else
				{
					pthread_mutex_lock(&Agent_Status_pmt);
					Agent_Status = json_status->valueint;
					pthread_mutex_unlock(&Agent_Status_pmt);
				}

				//查看是否需要更新新的agent版本
				if(json_publish_status != NULL)
				{
					if(json_publish_status->valueint == 1)
					{
						json_publish_version = cJSON_GetObjectItem(json_message, "publish_version");
						json_publish_url = cJSON_GetObjectItem(json_message, "publish_url");
						if(json_publish_version != NULL && json_publish_url != NULL)
						{
							//将版本号和url写入文件夹
							FILE *fp_version;
							fp_version = fopen("version.txt", "w");
							char str_version[105] = {0};
							strcpy(str_version, json_publish_version->valuestring);
							fputs(str_version, fp_version);
							fclose(fp_version);

							FILE *fp_url;
							fp_url = fopen("url.txt", "w");
							char str_url[105] = {0};
							strcpy(str_url, json_publish_url->valuestring);
							fputs(str_url, fp_url);
							fclose(fp_url);

							//将当前可用的IP和端口写入文件传递给更新进程
							FILE *fp_ip;
							fp_ip = fopen("download_ip.txt", "w");
							char ip_port_str[105] = {0};
							sprintf(ip_port_str, "%s %d", IPSTR, PORT);
							fputs(ip_port_str, fp_ip);
							fclose(fp_ip);

							//启动更新进程
							char result[4096] = {0};
							int ret = my_system("./Download_Update.sh", result);
							//sleep(1);
							if(pid > 0) //关闭Agent进程
							{
								kill(child_id, SIGKILL);
								waitpid(child_id, NULL, 0); //等待子线程关闭
							}
							system("pkill Agent");
							sleep(1);
							exit(0);//关闭守护进程
						}
					}
				}
        	}
        	else //解析错误
        	{
        		Daemon_Log("查询状态失败!\n");
        	}
        	free(json_cmd);
       		json_cmd = NULL;
       		free(json_code);
       		json_code = NULL;
       		free(json_message);
       		json_message = NULL;
        }

        //释放指针
       	free(json);
       	json = NULL;

	   	//关闭链接
	    close(sockfd);


	    //每隔2秒轮询一次、因为是阻塞式IO、服务端保存的状态变化或超时才给返回
	    //防止对服务器短时间造成大量请求
	    sleep(2);
	}
}

//Base64 编码
int Base64Encode( char *OrgString, char *Base64String, int OrgStringLen )
{
// OrgString 待编码字符串指针
// Base64String 保存编码结果字符串指针
// OrgStringLen 待编码字符串长度
    static char Base64Encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int Base64StringLen = 0;

    while( OrgStringLen > 0 )
    {
        *Base64String ++ = Base64Encode[(OrgString[0] >> 2 ) & 0x3f];
        if( OrgStringLen > 2 )
        {
            *Base64String ++ = Base64Encode[((OrgString[0] & 3) << 4) | (OrgString[1] >> 4)];
            *Base64String ++ = Base64Encode[((OrgString[1] & 0xF) << 2) | (OrgString[2] >> 6)];
            *Base64String ++ = Base64Encode[OrgString[2] & 0x3F];
        }
        else
        {
            switch( OrgStringLen )
            {
            case 1:
                *Base64String ++ = Base64Encode[(OrgString[0] & 3) << 4 ];
                *Base64String ++ = '=';
                *Base64String ++ = '=';
                break;
            case 2:
                *Base64String ++ = Base64Encode[((OrgString[0] & 3) << 4) | (OrgString[1] >> 4)];
                *Base64String ++ = Base64Encode[((OrgString[1] & 0x0F) << 2) | (OrgString[2] >> 6)];
                *Base64String ++ = '=';
                break;
            }
        }

        OrgString +=3;
        OrgStringLen -=3;
        Base64StringLen +=4;
    }

    *Base64String = 0;
    return Base64StringLen;
}
//////////////////////////////////////////////////////////////////////////////////////////
//Base64 解码
char GetBase64Value(char ch) //得到编码值
{
    if ((ch >= 'A') && (ch <= 'Z'))   // A ~ Z
        return ch - 'A';
    if ((ch >= 'a') && (ch <= 'z'))   // a ~ z
        return ch - 'a' + 26;
    if ((ch >= '0') && (ch <= '9'))   // 0 ~ 9
        return ch - '0' + 52;
    switch (ch)    // 其它字符
    {
    case '+':
        return 62;
    case '/':
        return 63;
    case '=':   //Base64 填充字符
        return 0;
    default:
        return 0;
    }
}
// 解码函数
int Base64Decode( char *OrgString, char *Base64String, int Base64StringLen, int bForceDecode )   //解码函数
{
// OrgString 保存解码结果字符串指针
// Base64String 待解码字符串指针
// Base64StringLen 待解码字符串长度
// bForceDecode 当待解码字符串长度错误时,是否强制解码
//     true   强制解码
//     false 不强制解码
    if( Base64StringLen % 4 && !bForceDecode ) //如果不是 4 的倍数,则 Base64 编码有问题
    {
        OrgString[0] = '\0';
		//printf("不是4的倍数，无法解析\n");
        return -1;
    }

    unsigned char Base64Encode[4];
    int OrgStringLen=0;

    while( Base64StringLen > 2 )   //当待解码个数不足3个时,将忽略它(强制解码时有效)
    {
        Base64Encode[0] = GetBase64Value(Base64String[0]);
        Base64Encode[1] = GetBase64Value(Base64String[1]);
        Base64Encode[2] = GetBase64Value(Base64String[2]);
        Base64Encode[3] = GetBase64Value(Base64String[3]);

        *OrgString ++ = (Base64Encode[0] << 2) | (Base64Encode[1] >> 4);
        *OrgString ++ = (Base64Encode[1] << 4) | (Base64Encode[2] >> 2);
        *OrgString ++ = (Base64Encode[2] << 6) | (Base64Encode[3]);

        Base64String += 4;
        Base64StringLen -= 4;
        OrgStringLen += 3;
    }

    return OrgStringLen;
}

//判断Agent进程是否存在
int Judge_Agent_Status()
{
	char buf[BUFSIZE] = {0};
	FILE *Agent_fp;
	int res = 0;

	if((Agent_fp = popen("pidof Agent", "r")) == NULL)
	{
		char log[1024] = {0};
		sprintf(log, "popen函数执行出错:%s\n", strerror(errno));
		Daemon_Log(log);
		return -1;
	}
	else
	{
		while(fgets(buf, sizeof(buf), Agent_fp));
		int len = strlen(buf);

		int i;
		agent_pid = 0;
		for(i = 0; i < len; i++)
		{
			if(buf[i] >= '0' && buf[i] <= '9')
			{
				agent_pid = agent_pid * 10 + buf[i] - '0';
			}
			else break;
		}

		if((res = pclose(Agent_fp)) == -1)
		{
			Agent_fp = NULL;
			Daemon_Log("popen关闭函数出错!\n");
			return -1;
		}
		else
		{
			Agent_fp = NULL;
			if(strlen(buf) != 0) return 1; //Agent进程存在
			else return 0; //Agent进程不存在
		}
	}
}

//system升级版、避免系统信号突然改变的坑
int my_system(char *cmd, char *result)
{
	FILE *pp = popen(cmd, "r"); //建立管道
	if (pp == NULL) {
        char log[BUFSIZE] = {0};
		sprintf(log, "popen函数执行出错:%s", strerror(errno));
		Daemon_Log(log);
		return -1;
    }
	char tmp[BUFSIZE] = {0};
    while(fgets(tmp, sizeof(tmp), pp) != NULL)
	{
		strcat(result, tmp);
	}

	int res = 0;
    if((res = pclose(pp)) == -1)//关闭管道
	{
		Daemon_Log("popen关闭函数出错!");
		return -1;
	}
	return res;
}

//判断Daemon是否已启动
void Daemon_RunAgain()
{

	char result[4096] = {0};
	int res = my_system("pidof Daemon", result);

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
	printf("count:%d\n", count);
	if(count > 1)
		exit(0);
}

int main()
{
	Daemon_RunAgain();

	int ppid = getpid();
	Daemon_Init(); //初始化数据

	//Create_Daemon(); // 创建守护进程
	Daemon_Http_Test();//测试网络情况
	Daemon_Register(); //注册


	if (pthread_mutex_init(&mut, NULL) != 0)
    {
        printf("mutex init error\n");
    }
    if (pthread_cond_init(&cond, NULL) != 0)
    {
        printf("cond init error\n");
    }

	//开一个线程对Agent状态进行轮询
	pthread_t http_thread;
	int ret = pthread_create(&http_thread, NULL, (void *)Daemon_RunStatus, NULL);
	if(ret != 0)//创建线程失败
	{
		//写入日志
		Daemon_Log("Daemon_RunStatus Thread create failed!");

		exit(EXIT_FAILURE);//退出程序、排查原因
	}

	pid = -1;
	child_id = -1;
	//主线控制轮询线程的销毁与创建以及控制Agent的开启和关闭


	int sec_count = 0;
	int now = 0;
	int pre = -1;
	while(1)
	{
		// 加锁防止异常
		int Status = Agent_Status;
		if(Status == 0)//关闭Agent
		{
			pthread_mutex_lock(&Agent_Status_pmt);
			Agent_Status = 5;
			pthread_mutex_unlock(&Agent_Status_pmt);
			//Agent_Status = 5;
			//检测Agent是否开启、防止多次关闭
			int res = Judge_Agent_Status();
			if(res == 0) //Agent不存在
			{
				//不做处理
			}
			else //Agent存在、关闭Agentl
			{
				if(pid > 0) //父进程
				{
					kill(child_id, SIGKILL);
					waitpid(child_id, NULL, 0); //等待子线程关闭
				}
			}
			//发送关闭成功消息
			char buf[BUFSIZE] = {0};
			sprintf(buf, "{\"cmd\":9009,\"run_status\":%d,\"version\":\"v_0.0.1\",\"token\":\"n7sbl64qb88l0xy\"}", Agent_Status);
			SendMessage(buf);

		}
		else if(Status == 10)//开启Agent
		{
			pthread_mutex_lock(&Agent_Status_pmt);
			Agent_Status = 15;
			pthread_mutex_unlock(&Agent_Status_pmt);
			//检测Agent是否关闭、防止多次开启
			int res = Judge_Agent_Status();
			if(res == 1) //Agent已存在
			{
				//不做处理
			}
			else
			{
				//发送开启成功消息
				char buf[BUFSIZE] = {0};
				sprintf(buf, "{\"cmd\":9007,\"run_status\":%d,\"version\":\"v_0.0.1\",\"token\":\"n7sbl64qb88l0xy\"}", Agent_Status);
				SendMessage(buf);
				Daemon_Log("fork daemon");
				pid = fork();
				if(pid < 0)
				{
					Daemon_Log("fork error");
				}
				else if(pid > 0)
				{
					child_id = pid;
				}
				else if(pid == 0)
				{
					execl("./Agent", "Agent", (char *)0);
					//子进程替换之后应该自动退出
					exit(0);
				}
			}
		}
		else if(Status == 5)//已关闭Agent
		{
			//复查确认Agent是否关闭
			int res = Judge_Agent_Status();
			if(res == 0) //Agent不存在
			{
				//不做处理
			}
			else //Agent存在、关闭Agent
			{
				if(pid > 0) //父进程
				{
					kill(child_id, SIGKILL);
					waitpid(child_id, NULL, 0); //等待子线程关闭
				}
			}
		}
		else if(Status == 15)//已开启Agent
		{
			//复查Agent是否开启
			int res = Judge_Agent_Status();
			if(res == 1) //Agent已存在
			{
				//不做处理
			}
			else
			{
				pid = fork();
				if(pid < 0)
				{
					Daemon_Log("fork子进程失败");
				}
				else if(pid > 0)
				{
					child_id = pid;
				}
				else if(pid == 0)
				{
					execl("./Agent", "Agent", (char *)0);
					//子进程替换之后应该自动退出
					exit(0);
				}
			}
		}
		else if(Status == -15) //卸载agent
		{
			Daemon_Log("卸载Agent!\n");
			//复查确认Agent是否关闭
			int res = Judge_Agent_Status();
			if(res == 0) //Agent不存在
			{
				//不做处理
			}
			else //Agent存在、关闭Agent
			{
				if(pid > 0) //父进程
				{
					kill(child_id, SIGKILL);
					waitpid(child_id, NULL, 0); //等待子线程关闭
				}
			}
			//关闭轮询线程
			thread_pause();
			sleep(1);//防止卸载消息比轮询先到达
			//通知服务端卸载成功并退出
			char buf[BUFSIZE] = {0};
			sprintf(buf, "{\"cmd\":9011,\"version\":\"v_0.0.1\",\"token\":\"n7sbl64qb88l0xy\"}");
			Daemon_Log(buf);
			SendMessage(buf);
			exit(0);
		}
		if(ppid != getpid()) //
		{
			Daemon_Log("出现异常子进程未关闭、进行关闭");
			exit(0);
		}

		sleep(2);//每隔1秒进行轮询、如果过快注意多次改变状态的问题

		sec_count++;
		if(sec_count % 3 == 0) //每6秒判断一次agent进程是否变成僵尸进程
		{
			int res = 0;
			res = Judge_Agent_Status();
			if(res == 1) //agent存在
			{
				FILE *agent_heart_fp; //判断agent是否是僵尸进程
				char cmdbuf[1024] = {0};
				printf("....................agent_pid:%d\n", agent_pid);
				sprintf(cmdbuf, "ps -ef | grep %d | grep Agent", agent_pid);
				if((agent_heart_fp = popen(cmdbuf, "r")) == NULL)
				{
					char log[1024] = {0};
					sprintf(log, "popen函数执行出错:%s\n", strerror(errno));
					Daemon_Log(log);
					exit(1);
				}
				else
				{
					char tempbuf[1024] = {0};
					char buf[4096] = {0};
					while(fgets(tempbuf, sizeof(tempbuf), agent_heart_fp) != NULL)
					{
						strcat(buf, tempbuf);
					}
					int len = strlen(buf);
					printf("len:%d result buf:%s\n", len, buf);

					if(strstr(buf, "<defunct>") > 0) //发现僵尸进程
					{
						Daemon_Log("Agent变成僵尸进程,关闭Agent");
						kill(child_id, SIGKILL);
						waitpid(child_id, NULL, 0); //等待子线程关闭
					}
					if((res = pclose(agent_heart_fp)) == -1)
					{
						agent_heart_fp = NULL;
						Daemon_Log("popen关闭函数出错!\n");
						exit(1);
					}
					else
					{
						agent_heart_fp = NULL;
					}
				}

			}
		}
	}

	return 0;
}
