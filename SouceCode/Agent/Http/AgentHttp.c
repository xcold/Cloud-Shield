
//数据监控线程、从plugin中执行各个python脚本插件、将数据上传大数据server

#include "./../PublicAPI.h"
#include "AgentHttp.h"

//#define IPSTR "10.17.64.213" //大数据内网测试地址
//#define PORT 39989  //大数据内网测试端口
//#define BUFSIZE 4096

//==================================================
//静态全局变量	作用域：本文件
//==================================================
//ip:port level
//192.168.0.1:8080 1
struct BigDataServerIP BigDataServerIP_List[1005]; //大数据服务端IP列表
static int BigDataServerIP_MaxNum = 0;


//http通信初始化
void Http_Init()
{
	memset(&BigDataServerIP_List, 0, sizeof(BigDataServerIP_List));

	//从配置文件中读取大数据服务端IP和端口
	FILE *fp;
	fp = fopen("./ServerIP/BigDataServerIP.ini", "r");
	if(fp == NULL)
	{
		Agent_Log("打开BigDataServerIP文件失败");
		exit(EXIT_FAILURE);
	}
	char buf[BUFSIZE] = {0};
	while(fgets(buf, sizeof(buf), fp) != NULL);
	int len = strlen(buf);
	if(len < 10)
	{
		Agent_Log("BigDataServerIP配置文件长度错误");
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
				Agent_Log("BigDataServerIP配置文件IP错误");
				exit(EXIT_FAILURE);
			}
			strcpy(BigDataServerIP_List[BigDataServerIP_MaxNum].ipstr, ipstr);

			BigDataServerIP_List[BigDataServerIP_MaxNum].port = port;
			if(BigDataServerIP_List[BigDataServerIP_MaxNum].port <= 1 || BigDataServerIP_List[BigDataServerIP_MaxNum].port >= 65535)
			{
				Agent_Log("BigDataServerIP配置文件端口错误");
				exit(EXIT_FAILURE);
			}

			BigDataServerIP_List[BigDataServerIP_MaxNum].level = buf[i] - '0';
			if(BigDataServerIP_List[BigDataServerIP_MaxNum].level < 1 || BigDataServerIP_List[BigDataServerIP_MaxNum].level > 9)
			{
				Agent_Log("BigDataServerIP配置文件优先级错误");
				exit(EXIT_FAILURE);
			}
			BigDataServerIP_MaxNum++;
		}
	}
	if(BigDataServerIP_MaxNum <= 0)
	{
		Agent_Log("BigDataServerIP配置文件未找到IP列表");
		exit(EXIT_SUCCESS);
	}
	printf("大数据IP：%s:%d %d\n", BigDataServerIP_List[0].ipstr, BigDataServerIP_List[0].port, BigDataServerIP_List[0].level);
}


//负责向大数据post监控数据
void AgentHttpPost_fun(char* sendmsg)
{
	//初始化主机名和端口
	char host_name[256];
	struct hostent *nlp_host;
	strcpy(host_name, "chc.bd.duoyi.com");
	int port = 80;
	//解析域名
	while((nlp_host = gethostbyname(host_name)) == 0)
	{
		printf("Resolve Error!\n");
	}

	printf("http post\n");
	//connect
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t len;
	fd_set t_set1;
    //struct timeval  tv;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        printf("创建网络连接失败,本线程即将终止---socket error!\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	int PORT = BigDataServerIP_List[0].port;
    servaddr.sin_port = htons(port);
	char IPSTR[32] = {0};
	strcpy(IPSTR, BigDataServerIP_List[0].ipstr);
    if (inet_pton(AF_INET, inet_ntoa(*(struct in_addr*)nlp_host->h_addr), &servaddr.sin_addr) <= 0 ){
        printf("创建网络连接失败,本线程即将终止--inet_pton error!\n");
        exit(0);
    }

	//自动尝试3次连接 每次间隔10S
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        printf("连接到服务器失败,connect error!\n");
        exit(0);
    }

    printf("与远端建立了连接\n");

    char *str;
    str = (char *)malloc(128);
    len = strlen(sendmsg);
    sprintf(str, "%d", len);

	char sendbuf[4096] = {0};
	char recvbuf[4096] = {0};
	memset(sendbuf, 0, sizeof(sendbuf));
	memset(recvbuf, 0, sizeof(recvbuf));
	//http消息构造
	sprintf(sendbuf, "POST / HTTP/1.1\n");
	strcat(sendbuf, "Host: chc.bd.duoyi.com\n");
	strcat(sendbuf, "Content-Type: application/json\n");

	strcat(sendbuf, "Content-Length: ");
	strcat(sendbuf, str);
	free(str);
	str = NULL;
	strcat(sendbuf, "\n\n");

	strcat(sendbuf, sendmsg);
	strcat(sendbuf, "\r\n\r\n");
	printf("%s\n", sendbuf);
	//char tempbuf[4096] = {0};

	//发送http请求
	int ret = send(sockfd, sendbuf, strlen(sendbuf), 0);//发送的时候用strlen
	if(ret < 0)
	{
		printf("http send error!");
	}
	else
	{
		printf("消息发送成功, 共发送了%d个字节\n\n", ret);
	}
	
	FD_ZERO(&t_set1);
	FD_SET(sockfd, &t_set1);
	
	//收到返回值、做重发处理和超时处理
	ret = recv(sockfd, recvbuf, sizeof(sendbuf), 0);//接收的时候用sizeof
	if(ret < 0)
	{
		printf("http recv error!");
	}
	printf("recvbuf:%s\n", recvbuf);
	//如果返回值状态不为200 OK则重发
	if(strstr(recvbuf, "200 OK") != NULL)
	{
		printf("send ok!\n");
	}
	else
	{
		//重发
		send(sockfd, sendbuf, strlen(sendbuf), 0);
	}

   	//关闭链接
    close(sockfd);
}
