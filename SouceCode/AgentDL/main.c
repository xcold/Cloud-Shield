//C语言实现下载更新模块

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

#include "Log/Log.h"

#define BUFSIZE 4096
#define MAXDATASIZE 4096

//#define IPSTR "10.17.17.98"
//#define PORT 38080

char IPSTR[32] = {0};
int PORT = 0;

#define TFILE "data_from_socket.txt"

int Base64Decode( char *OrgString, char *Base64String, int Base64StringLen, int bForceDecode );

char GetBase64Value(char ch); //得到编码值

int Base64Encode( char *OrgString, char *Base64String, int OrgStringLen );


//int sockfd = 0;
char update_file_name[BUFSIZE];
char url[BUFSIZE];
char version[BUFSIZE];

//记录更新下载程序的日志
void Download_Log(char *buf)
{
	printf("Log_AgentDL:%s\n", buf);
	Log_Add("Log_AgentDL", buf);
}

//system升级版、避免系统信号突然改变的坑
int my_system(char *cmd, char *result)
{
	FILE *pp = popen(cmd, "r"); //建立管道
	if (pp == NULL) {
        char log[BUFSIZE] = {0};
		sprintf(log, "popen函数执行出错:%s", strerror(errno));
		Download_Log(log);
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
			Download_Log("运行结果输出太长，无法展示完");
			break;
		}
		strcat(result, tmp);
	}

	int res = 0;
    if((res = pclose(pp)) == -1)//关闭管道
	{
		Download_Log("popen关闭函数出错!");
		return -1;
	}
	return res;
}


//从url.txt文件中获取更新文件地址并提取出文件名称
void Get_Update_Url()
{
	memset(url, 0, sizeof(url));
	FILE *fp_url;
	fp_url = fopen("url.txt", "r");
	while(fgets(url, sizeof(url), fp_url) != NULL);
	int len = strlen(url);
	int i = 0;
	for(i = 0; i < len; i++)
	{
		if(url[i] == '\n')
		{
			url[i] = '\0';
			break;
		}
	}
	len = strlen(url);
	int temp_i = 0;
	for(i = len - 1; i >= 0; i--)
	{
		if(url[i] == '/')
		{
			temp_i = i + 1;
			break;
		}
	}
	memset(update_file_name, 0, sizeof(update_file_name));
	int j = 0;
	for(i = temp_i; i < len; i++)
	{
		update_file_name[j++] = url[i];
	}
	printf("url:%s\n", url);
}

//接收从url下载的文件
int Process_Conn_Client(int sockfd)
{
	fd_set readset, writeset;
	int check_timeval = 1;
	struct timeval timeout = {check_timeval, 0}; //阻塞式select,阻塞1秒,1秒轮询
	int maxfd, fp, ret, cir_count = 0;
	FILE *fp_dy;
	fp_dy = fopen("Agent.tar", "wb+x");

	if((fp = open(TFILE, O_WRONLY)) < 0)
	{
		perror("fopen:");
		return 1; //打开文件失败
	}

	int total = 0;
	int header = 0;
	//等待服务器的请求进行处理
	while(1)
	{
		FD_ZERO(&readset);				//每次循环都要清空集合，否则不能检测描述符变化
		FD_SET(sockfd, &readset);		//添加描述符
		FD_ZERO(&writeset);
		FD_SET(fp, &writeset);

		maxfd = sockfd > fp ? (sockfd + 1) : (fp + 1); //描述符最大值加1

		ret = select(maxfd, &readset, NULL, NULL, &timeout);   // 非阻塞模式

        switch(ret)
        {
            case -1: //出错
                Download_Log("socket 链接出错！\n");
                sleep(5);
                return -1; //运行
                break;
            case 0:  //超时、单位时间内没有数据、如果2分钟一直未接收到服务端响应则自动断开重连
                break;
            default:  //>0 就绪描述字的正数目
                if(FD_ISSET(sockfd, &readset))  //测试sock是否可读，即是否网络上有数据
                {
                    char recvbuf[MAXDATASIZE] = {0};

                    ret = recv(sockfd, recvbuf, sizeof(recvbuf), 0); //接收服务端来的数据
                    total = total + ret;
                    if(ret == 0) //更新完成或网络问题没有数据
                    {
                    	goto end;
                    }

                    if(ret == 1) //服务端socket已关闭
                    {
                        Download_Log("服务端已关闭\n");
                        goto end; //结束连接过程
                    }
                    else if(ret == -1) //接收到的数据不完整
                    {
                        //返回接收错误信息、看作有心跳回应
                        break;
                    }
                    else if(ret == -2)
                    {
                        //服务端异常断开
                        Download_Log("服务端异常断开\n");
                        goto end;
                    }
                    else //成功解析出数据
                    {
                    	header++;
                    	if(header >= 2)
                    	{
	                        if(header == 2)//清除HTTP header头
	                        {
	                        	char recvmsg[BUFSIZE] = {0};
	                        	int j = 0, k = 0;
	                        	int flag = 0;
	                        	int temp_k = 0;
	                        	for(k = 4; k < ret; k++)
	                        	{
	                        		if(flag == 1)
	                        		{
	                        			if(temp_k == 0) temp_k = k;
	                        			recvmsg[j++] = recvbuf[k];
	                        		}
	                        		else if(recvbuf[k] == '\n' && recvbuf[k - 1] == '\r' && recvbuf[k -2] == '\n' && recvbuf[k -3] == '\r')
	                        		{
	                        			flag = 1;
	                        		}
	                        		else if(recvbuf[k] == '\n' && recvbuf[k - 1] == '\n')
	                        		{
	                        			flag = 1;
	                        		}
	                        	}
	                        	fwrite(recvmsg, ret - temp_k, 1, fp_dy);
	                        	fflush(fp_dy);
	                        }
	                        else
	                        {
	                        	fwrite(recvbuf, ret, 1, fp_dy);
	                        	fflush(fp_dy);
	                        }
                    	}
                    }

                    if(FD_ISSET(fp, &writeset))//测试fp是否可写
                    {
                    	write(fp, recvbuf, strlen(recvbuf)); //不是用fwrite
                    }
                    break;
                }
        }
        timeout.tv_sec = check_timeval;    // 必须重新设置，因为超时时间到后会将其置零

        cir_count++;
    }

    end:
    	fflush(fp_dy);
    	fclose(fp_dy);
    	close(fp);//关闭文件！！！这里是close不是fclose
        return 0;
}

//0表示下载成功 1表示下载失败
int Download_New_File()
{
	int Repeat_Connect = 0;
	int Repeat_Send = 0;
	char sendbuf[BUFSIZE];

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
	        Download_Log(log);
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
	        Download_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

		//自动尝试3次连接 每次间隔10S
	    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			char log[1024] = {0};
			sprintf(log, "连接到服务器失败,即将重新链接--connect error! 已连续重连%d次\n", Repeat_Connect);
	        Download_Log(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

	    Download_Log("与服务端建立了连接...\n");

		//清空
		memset(sendbuf, 0, sizeof(sendbuf));

		//http消息头构造
		sprintf(sendbuf, "GET %s HTTP/1.1\n", url);
		printf("sendbuf:%s", sendbuf);
		strcat(sendbuf, "Host: 10.17.17.98:38080\n");
		strcat(sendbuf, "Accept-Language: UTF-8");
		strcat(sendbuf, "User-Agent: Debian");
		strcat(sendbuf, "Content-Type: application/json\n");

		char content_len[105] = {0};
		sprintf(content_len, "Content-Length: %d", 0);
		strcat(sendbuf, content_len);
		strcat(sendbuf, "\n\n"); //间隔一行表示下面是内容


		//发送GET请求
		ret = send(sockfd, sendbuf, strlen(sendbuf), 0);//发送的时候用strlen
		if(ret < 0)//send数据错误重发
		{
			char log[1024] = {0};
			sprintf(log, "http send error!--已连续重发%d次\n", Repeat_Send);
			Download_Log(log);
			close(sockfd); //关闭套接字
			continue;
		}
		else
		{
			Repeat_Send = 0;
			char log[1024] = {0};
			sprintf(log, "消息发送成功, 共发送了%d个字节\n\n", ret);
			Download_Log(log);
		}

		Process_Conn_Client(sockfd);

		close(sockfd); //关闭套接字
		break;
	}
}

//获取版本号
void Get_Version(char *version)
{
	FILE *fp_ver;
	fp_ver = fopen("version.txt", "r");
	while(fgets(version, sizeof(version), fp_ver) != NULL);
	int len = strlen(version);
	int i;
	for(i = 0; i < len; i++)
	{
		if(version[i] == '\n')
		{
			version[i] = '\0';
			break;
		}
	}
	fclose(fp_ver);
	return;
}

//解压并启动下载的更新文件
//0表示更新成功 1表示更新失败
int Extract_File()
{
	//更改当前文件名称 加上版本号后缀 Agent-version
	memset(version, 0, sizeof(version));
	char result[BUFSIZE] = {0};
	char cmd[1024] = {0};
	int ret = 0;

	Get_Version(version);
	//mv Agent Agent-version
	sprintf(cmd, "mv /usr/local/Agent /usr/local/Agent-%s", version);
	//int ret = my_system(cmd, result);
	ret = system(cmd);
	//if(ret <= 0 || ret == 127) return 1;

	memset(cmd, 0, sizeof(cmd));
	memset(result, 0, sizeof(result));
	//sprintf(cmd, "tar -xvf Agent.tar -C /usr/local/");
	//ret = my_system(cmd, result);
	sleep(1);//等一下再解压、以免解压失败
	ret = system("tar -xvf Agent.tar -C /usr/local/");
	printf("解压压缩包成功\n");
	sleep(3);//解压有一个过程、等下再启动
	//if(ret <= 0 || ret == 127) return 1;

	//Start_New_Daemon
	//ret = my_system("./Start_New_Daemon.sh", result);
	sprintf(cmd, "/usr/local/Agent-%s/Start_New_Daemon.sh", version);
	ret = system(cmd);
	//if(ret <= 0 || ret == 127) return 1;
	printf("启动Start_New_Daemon.sh成功\n");
	//判断更新是否成功 pidof Daemon
	memset(cmd, 0, sizeof(cmd));
	memset(result, 0, sizeof(result));
	sprintf(cmd, "pidof Daemon");
	sleep(1);//等待1秒 daemon启动
	ret = my_system("pidof Daemon", result);
	int len = strlen(result);
	if(len == 0)//没有启动成功
	{
		printf("启动新的守护进程失败\n");
		//删除解压文件和压缩包
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "rm -rf /usr/local/Agent-%s/Agent.tar", version);
		system(cmd);
		system("rm -rf /usr/local/Agent");
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "mv /usr/local/Agent-%s /usr/local/Agent", version);
		//还原文件名称
		system(cmd);

		//启动原来的Agent
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "/usr/local/Agent/Start_Old_Daemon.sh");
		system(cmd);
		return 1;
	}
	//启动成功
	printf("启动新的守护进程成功\n");
	printf("len:%d  result:%s\n", len, result);
	//删除压缩包和旧文件
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "rm -rf /usr/local/Agent-%s", version);
	system(cmd);
	return 0;
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
	        printf(log);
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
	        printf(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

		//自动尝试3次连接 每次间隔10S
	    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			char log[1024] = {0};
			sprintf(log, "连接到服务器失败,即将重新链接--connect error! 已连续重连%d次\n", Repeat_Connect);
	        printf(log);
	        sleep(3); //暂停3秒、防止无限重连消耗资源
	        Repeat_Connect++;
	        close(sockfd); //关闭套接字
	        continue; //网络断开再次重新链接
	    }

	    printf("与服务端建立了连接...\n");

		printf(sendmsg);

		char Encodemsg[4096] = {0};
		Base64Encode(sendmsg, Encodemsg, strlen(sendmsg));//数据加密、只加密body部分
		len = strlen(Encodemsg);

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
			printf(log);
			close(sockfd); //关闭套接字
			continue;
		}
		else
		{
			Repeat_Send = 0;
			char log[1024] = {0};
			sprintf(log, "消息发送成功, 共发送了%d个字节\n\n", ret);
			printf(log);
		}

		close(sockfd); //关闭套接字
		break;//发送成功则退出
	}
}

void Get_Update_IP()
{
	FILE *fp_ip;
	fp_ip = fopen("download_ip.txt", "r");
	char ip_port_str[105] = {0};
	while(fgets(ip_port_str, sizeof(ip_port_str), fp_ip) != NULL);
	sscanf(ip_port_str, "%s %d", IPSTR, &PORT);
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


int main()
{
	//读取IP和端口
	Get_Update_IP();

	//读取url
	Get_Update_Url();

	//下载文件
	Download_New_File();

	//更新文件
	int ret = Extract_File();
	printf("ret: %d\n", ret);

	//通知服务端更新成功或失败
	if(ret == 1) //失败
	{
		char cmd[1024] = "{\"cmd\":9015}";
		SendMessage(cmd);
	}
	else
	{
		char cmd[4096] = {0};
		sprintf(cmd, "{\"cmd\":9013,\"version\":\"%s\"}", version);
		printf("9013cmd:%s\n", cmd);
		SendMessage(cmd);
	}
	printf("退出更新进程...\n");
	return 0;
}
