//提取对应的脚本结果发送给云盾服务端和大数据

#include "./../PublicAPI.h"
#include "PluginSendData.h"
#include "PluginOperate.h"
#include "./../Tcp/Tcp.h"

//==================================================
//引用全局变量  作用域：本文件
//==================================================
extern struct PluginNode plugin_list[MAXN];//插件结构列表
extern int plugin_maxnum;

//==================================================
//内部函数声明  作用域：本文件
//==================================================
void PluginMsg(char* name, char* value, int rate);

void GetNowTime(char* NowTime);

void GetIPAddress(char* IPAddress);





//按json格式整合
//整合的时候判断是否所有脚本插件已运行出结果、将整合的消息保存在一个PluginMsg.txt文件里
void PluginMsg(char* name, char* value, int rate)
{
	printf("pluginMsg function\n");
    //json
    /*
    [{
	"header":{}, 
	"body":"time\tendpoint\tbd_monitor_1.0\tyunwei\tmetric\tvalue\tcounterType\tstep\ttag"
    }]
    */
    char NowTime[50] = {0};
    GetNowTime(NowTime);

    char IPAddress[50] = {0};
    GetIPAddress(IPAddress);

    char str_json[4096] = {0};
    sprintf(str_json, "[{\"headers\":{}, ");
    //数据格式
	char tempbuf[4096] = {0};
	sprintf(tempbuf, "\"body\":\"%s\t%s\tbd_monitor_1.0\tyunwei\t%s\t%s\tgauge\t%d\"}]", NowTime, IPAddress, name, value, rate);
    strcat(str_json, tempbuf);

	printf("%s\n", str_json);
    //将生成的json调用http转发给大数据
    AgentHttpPost_fun(str_json);
	

/*
    FILE *fp;

	if((fp = fopen("PluginMsg.txt", "w")) == NULL)
	{
		printf("不能打开PluginMsg.txt文件");
	    fclose(fp);
		//exit(0);
	}
	else
	{
		fwrite(str_json, strlen(str_json), fp);
		fclose(fp);
	}
*/
}

/*
int Collect_ok()
{
	int flag = 1;
	for(int i = 0; i < plugin_flag[python_files]; i++)
	{
		if(plugin_flag[python_files] == 0 && plugin_num[i] == 1)
			flag = 0;
	}
	return flag;
}
*/


//2016-12-21 16:49:51
void GetNowTime(char* NowTime)
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(NowTime, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", 1900+timenow->tm_year,1+timenow->tm_mon,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
}

void GetIPAddress(char* IPAddress)
{
	struct ifaddrs * ifAddrStruct=NULL;  
    void * tmpAddrPtr=NULL;  
  
    getifaddrs(&ifAddrStruct);  
  
    while (ifAddrStruct!=NULL)   
    {  
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)  
        {   // check it is IP4  
            // is a valid IP4 Address  
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;  
            char addressBuffer[INET_ADDRSTRLEN];  
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);  
            printf("%s\n", addressBuffer);   
            strcpy(IPAddress, addressBuffer);
        }  
        /*
        else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)  
        {   // check it is IP6  
            // is a valid IP6 Address  
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;  
            char addressBuffer[INET6_ADDRSTRLEN];  
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);  
            printf("%s\n", addressBuffer);   
        }   
        */
        ifAddrStruct = ifAddrStruct->ifa_next;  
    }  
    //sprintf(IPAddress, "%s", addressBuffer);
}

//插件结果提取
//[('cpu2.irq', '0'), ('cpu0.guest_nice', '0'), ('cpu1.system', '50884')]
void Plugin_ResSend(int p)
{
	printf("pluginresget function*******\n");
    //读取存储在buf中
    //char buf[BUFSIZE] = {0};

    int direction = plugin_list[p].direction;
    if(direction == 1) //结果返回给云盾服务端
    {
		char sendbuf[4096] = "{\"cmd\":1107,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":";
		sprintf(sendbuf,"%s\"%s\",\"code\":1,\"message\":\"%s\"}}",sendbuf,plugin_list[p].name,plugin_list[p].result);
		if(strlen(sendbuf) >= BUFSIZE)
		{
			Agent_Log("发送消息过长!");
			sprintf(sendbuf, "{\"cmd\":1107,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":\"%s\",\"code\":1,\"message\":\"返回结果过长，不显示\"}}", plugin_list[p].name);
		}
        Agent_Log(sendbuf);
        SendMessage(sendbuf);
    }
	else
	{
		char sendbuf[4096] = "{\"cmd\":1107,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":";
		sprintf(sendbuf,"%s\"%s\",\"code\":1,\"message\":\"结果发往大数据，不返回服务端\"}}",sendbuf,plugin_list[p].name);
		Agent_Log(sendbuf);
        SendMessage(sendbuf);
	}

	//判断脚本是否已执行完次数
    if(plugin_list[p].run_times == 0 || plugin_list[p].runonce == 1)
    {
        //修改成功返回消息
        char sendbuf[4096] = {0};
        sprintf(sendbuf,"{\"cmd\":1102,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":\"%s\"}}",plugin_list[p].name);
            
        SendMessage(sendbuf);
    }

    int rate = plugin_list[p].rate;

    if((strcmp(plugin_list[p].name, "gpu") == 0) || (strcmp(plugin_list[p].name, "memory") == 0))
    {
        //读取之后提取对应的值转化成json格式逐个发送
        char buf[BUFSIZE] = {0};

        strcpy(buf, plugin_list[p].result);
        Agent_Log("脚本结果发往大数据");
        Agent_Log(buf);
        int i = 0;
        char name[105] = {0};
        char value[105] = {0};
        while(1)
        {
            if(buf[i] == '(')
            {
                i = i + 3;
                int name_id = 0;
                int value_id = 0;
                memset(name, '\0', sizeof(name));
                memset(value, '\0', sizeof(value));
                while(buf[i] != '\'' && buf[i] != '\\')
                {
                    name[name_id++] = buf[i];
                    i++;
                }
                i = i + 6;
                while(buf[i] != '\'' && buf[i] != '\\')
                {
                    value[value_id++] = buf[i];
                    i++;
                }
                //把提取的结果按json格式整合
                PluginMsg(name, value, rate);
                //name-value一对键值
            }
            if(buf[i] == ']' || buf[i] == '\0')
            {
                break;
            }
            i++;
        }
    }
}
