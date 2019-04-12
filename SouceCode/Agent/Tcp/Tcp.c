//与云盾服务端TCP通信模块


#include "./../PublicAPI.h"
#include "Tcp.h"
#include "./../Log/Log.h"
#include "./../Plugin/Plugin.h"
#include "./../SolveMsg/SolveMsg.h"
#include "./../cJSON/cJSON.h"
#include "./../AES/MKAES.h"
#include "./../AgentManage/AgentManage.h"

//==================================================
//数据定义  作用域：本文件
//==================================================

#define TFILE "data_from_socket.txt" //从服务端接收的数据进行存储
#define MAXDATASIZE 4096 //定义单次接收数据最大值

struct AgentNode AgentState;//实例化一个agent状态
int sockfd = 0; //套接字符、全局声明方便引用

extern struct AgentNode AgentHealthStatus;

//==================================================
//静态全局变量    作用域：本文件
//==================================================

static struct YunDunServerIP YunDunServerIP_List[1005]; //云盾服务端IP列表
static int YunDunServerIP_MaxNum = 0;
//==================================================
//对内声明函数    作用域：本文件
//==================================================
void Agent_State();

int Create_Connect();

int Process_Conn_Client();

void SendByteChange(BYTE* buf, int len);

int RecvByteChange(BYTE* buf); //-1表示接收到的数据错误、其他表示接受到的数据长度

int SendAes(BYTE *buf);

void RecvAes(char *buf, int len);



//tcp通信初始化
void Tcp_Init()
{
    memset(&YunDunServerIP_List, 0, sizeof(YunDunServerIP_List));
    //从配置文件中读取云盾服务端IP和端口
    FILE *fp;
    fp = fopen("./ServerIP/YunDunServerIP_Tcp.ini", "r");
    if(fp == NULL)
    {
        Agent_Log("打开YunDunServerIP文件失败");
        exit(EXIT_FAILURE);
    }
    char buf[BUFSIZE] = {0};
    char tmp[BUFSIZE] = {0};
    int len = 0;
    while(fgets(tmp, sizeof(tmp), fp) != NULL)
    {
        int len_tmp = strlen(tmp);
        len = len + len_tmp;
        if(len > 3500)
        {
            Agent_Log("运行结果输出太长，无法展示完");
            break;
        }
        strcat(buf, tmp);
    }
    len = strlen(buf);
    if(len < 10)
    {
        Agent_Log("YunDunServerIP配置文件长度错误");
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
            strcpy(YunDunServerIP_List[YunDunServerIP_MaxNum].ipstr, ipstr);

            YunDunServerIP_List[YunDunServerIP_MaxNum].port = port;
            if(YunDunServerIP_List[YunDunServerIP_MaxNum].port <= 1 || YunDunServerIP_List[YunDunServerIP_MaxNum].port >= 65535)
            {
                Agent_Log("YunDunServerIP配置文件端口错误");
                exit(EXIT_FAILURE);
            }

            YunDunServerIP_List[YunDunServerIP_MaxNum].level = buf[i] - '0';
            if(YunDunServerIP_List[YunDunServerIP_MaxNum].level < 1 || YunDunServerIP_List[YunDunServerIP_MaxNum].level > 9)
            {
                Agent_Log("YunDunServerIP配置文件优先级错误");
                exit(EXIT_FAILURE);
            }
            YunDunServerIP_MaxNum++;
        }
    }
    if(YunDunServerIP_MaxNum <= 0)
    {
        Agent_Log("BigDataServerIP配置文件未找到IP列表");
        exit(EXIT_SUCCESS);
    }
	
    char log[1024] = {0};
    printf("YunDunServerIP_MaxNum:%d\n", YunDunServerIP_MaxNum);
    printf("服务端IP：%s:%d %d\n", YunDunServerIP_List[0].ipstr, YunDunServerIP_List[0].port, YunDunServerIP_List[0].level);
    printf("服务端IP：%s:%d %d\n", YunDunServerIP_List[1].ipstr, YunDunServerIP_List[1].port, YunDunServerIP_List[1].level);
}

//模拟心跳、定时发送agent的健康状态信息
void Agent_State()
{
	float agent_cpu = AgentHealthStatus.agent_cpu;
	float agent_memory = AgentHealthStatus.agent_memory;
	float plugin_cpu = 0.0;
	float security_cpu = 0.0;
	float machine_cpu = AgentHealthStatus.machine_cpu;
	float machine_memory = AgentHealthStatus.machine_memory;

	//从agent_state结构体中读取各项数据进行整合
	char sendbuf[MAXDATASIZE] = "{\"cmd\":1000,\"token\":\"n7sbl64qb88l0xy\",\"json\":{";
	sprintf(sendbuf, "%s\"agent_cpu\":%f,\"machine_cpu\":%f,\"machine_memory\":%f,\"version\":\"v_0.0.1\",\"security_cpu\":%f,\"plugin_cpu\":%f,\"agent_memory\":%f,\"host_time\":\"2017-03-09 19:40:08\"}}", \
				sendbuf, agent_cpu, machine_cpu, machine_memory, security_cpu, plugin_cpu,agent_memory);

	Agent_Log(sendbuf);
	int ret = SendMessage(sendbuf);
}

int Create_Connect(int connect_id)
{
    struct sockaddr_in server_addr; //描述服务器的地址
    struct hostent *host;

	char IPSTR[32] = {0};
	strcpy(IPSTR, YunDunServerIP_List[connect_id].ipstr);
	int PORT = 0;
	PORT = YunDunServerIP_List[connect_id].port;
	
    //目标IP格式转换
    if((host = gethostbyname(IPSTR)) == NULL)
    {
        Agent_Log("Gethostname error\n");
        return 1;
    }

    /* 客户程序开始建立 sockfd描述符 */ 
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // AF_INET:Internet;SOCK_STREAM:TCP
    {
		char log[1024] = {0};
		sprintf(log, "Socket Error:%s\n", strerror(errno));
        Agent_Log(log);
        return 1;
    }

    /* 客户程序填充服务端的资料 */ 
    bzero(&server_addr, sizeof(server_addr)); // 初始化,置0
    server_addr.sin_family = AF_INET; // IPV4
    server_addr.sin_port = htons(PORT); // (将本机器上的short数据转化为网络上的short数据)端口号
    server_addr.sin_addr = *((struct in_addr *)host->h_addr); // IP地址
    
    /* 客户程序发起连接请求 */ 
    if(connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1) 
    {
		char log[1024] = {0};
		sprintf(log, "Connect Error:%s\n", strerror(errno));
        Agent_Log(log); 
        return 1; //连接失败
    }
    
    return 0; //连接成功
}

//TCP通信线程线程函数
void Server_Interactive()
{
    int connect_id = 0;
    //网络断开自动重新连接
    while(1)
    {
        connect_id = connect_id % YunDunServerIP_MaxNum;

        int ret = 0;
        ret = Create_Connect(connect_id); //建立socket连接
        if(ret == 0)
        {
            //连接成功
            Agent_Log("connect success\n");
        }
        else
        {
            //连接失败
            Agent_Log("connect failed\n");
            connect_id++;
            sleep(3);//暂停3s换个IP连接
            continue;
        }
		printf("进行通信....\n");
        //通信处理过程
        Process_Conn_Client();

        //结束通讯
        close(sockfd);
        
        sleep(5); //5秒钟以后自动重新连接服务端
    }
}


//消息发送函数
int SendMessage(char* buf)
{
    int len = SendAes(buf);

    SendByteChange(buf, len);
    
    if(send(sockfd, buf, len + 6, 0) == -1)
    {
        return -1;
    }
    return 0;
}

//消息接收函数
int RecvMessage(char* buf)
{
    int recvbytes = recv(sockfd, buf, MAXDATASIZE, MSG_DONTWAIT);
	printf("接收到的消息：%s\n", buf);
    if(recvbytes == 0) return 1; //服务端socket已经正常关闭
    if(recvbytes < 0) //不处理会导致segmentation fault
    {
        Agent_Log("recv error ..............\n");
        return -2;
    }
    int ret = RecvByteChange(buf);
    
    RecvAes(buf, ret);
    
    if(ret == -1) return -1; //信息读取校验失败
    return 0;
}

int Process_Conn_Client()
{
    fd_set readset, writeset;
    int check_timeval = 1;
    struct timeval timeout = {check_timeval, 0}; //阻塞式select,阻塞1秒，1秒轮询
    int maxfd, fp, ret, cir_count = 0;

    if((fp = open(TFILE, O_WRONLY)) < 0)
    {
        perror("fopen:");
        return 1; // 打开文件失败
    }

    //等待服务器的请求进行处理
    while (1)
    {
        //Agent_Log("wwwwwwwwwwwwwwwwww\n");
        FD_ZERO(&readset);            //每次循环都要清空集合，否则不能检测描述符变化
        FD_SET(sockfd, &readset);     //添加描述符       
        FD_ZERO(&writeset);
        FD_SET(fp, &writeset);

        maxfd = sockfd > fp ? (sockfd+1) : (fp+1);    //描述符最大值加1

        ret = select(maxfd, &readset, NULL, NULL, &timeout);   // 非阻塞模式
        
        switch(ret)
        {
            case -1: //出错
                Agent_Log("socket 链接出错！\n");
                sleep(5);
                return -1; //运行
                break;
            case 0:  //超时、单位时间内没有数据、如果2分钟一直未接收到服务端响应则自动断开重连
                break;
            default:  //>0 就绪描述字的正数目
                if(FD_ISSET(sockfd, &readset))  //测试sock是否可读，即是否网络上有数据
                {
                    BYTE recvbuf[MAXDATASIZE] = {0};

                    ret = RecvMessage(recvbuf); //接收服务端来的数据
                    if(ret == 1) //服务端socket已关闭
                    {
                        Agent_Log("服务端已关闭\n");
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
                        Agent_Log("服务端异常断开\n");
                        goto end;
                    }
                    else //成功解析出数据
                    {
                        //根据解析
                        Tcp_Msg_Solve(recvbuf);
                    }

                    //将接收到的数据写入文件中
                    if(FD_ISSET(fp, &writeset)) //测试fp是否可写
                    {
                        write(fp, recvbuf, strlen(recvbuf));   // 不是用fwrite
                    }
                    
                    break;
                }
        }
        timeout.tv_sec = check_timeval;    // 必须重新设置，因为超时时间到后会将其置零

        if(cir_count % 30 == 0)//每30秒发送一次心跳消息
        {
			printf("发送心跳信息...\n");
            Agent_State();
        }
        if(cir_count == 5)
        {
			printf("同步脚本信息...\n");
            AgentPluginMsgToServer();//同步脚本信息
        }
        cir_count++;
    }

    end:
        close(fp); //关闭文件 !!!这里是close不是fclose
        return 0;
}


//byte字节接收转换
int  RecvByteChange(BYTE* buf)
{
    BYTE buffer[MAXDATASIZE] = {0};
    int cnt = 0;
    int val = 0; //传输内容的长度
    while(cnt <= 3)
    {
        val = val << 8;
        val += buf[cnt++];
    }
    if(val == 0) return -1; //传输数据长度为0
    int flag = buf[4]; //0表示不加密，1表示base64加密

    int i;
    for(i = 0; i < val; i++)
    {
        //if(buf[i + 5] == 0) return -1; //数据长度不一致 AES加密之后有可能出现0
        buffer[i] = buf[i + 5];
    }
    buffer[val] = 0; //增加一个结束符
    memcpy(buf, buffer, val + 1);
    return val;
}


//byte字节发送转换
void SendByteChange(BYTE* buf, int len)
{
    int val = len;
    int size = len + 6;//实际长度
    BYTE buffer[MAXDATASIZE] = {0};
    int cnt = 3;
    while(val > 0 && cnt >= 0)
    {
        buffer[cnt--] = val;
        val = val >> 8;
    }
    buffer[4] = 0; //0表示不加密，1表示base64加密

    int i;
    for(i = 0; i < len; i++)
    {
        buffer[i + 5] = buf[i];
    }
    buffer[i + 5 + len] = 0; //0为结束符
    memcpy(buf, buffer, size);//将buffer数据通过内存拷贝到buf中
}

int SendAes(BYTE *buf)
{
    int srclen = strlen((char *)buf);
    char *pcEncodeDst = 0;
    int dstlen = 0;
    AESEncode(buf, srclen, &pcEncodeDst, &dstlen);
    memcpy(buf, pcEncodeDst, dstlen);
    return dstlen;
}

void RecvAes(char *buf, int len)
{
    char *pcDecodeDst = 0;
    int dstlen = 0;
    AESDecode(buf, len, &pcDecodeDst, &dstlen);
    pcDecodeDst[dstlen] = '\0';
    memcpy(buf, pcDecodeDst, dstlen + 1);
}