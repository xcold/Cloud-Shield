//插件的增删改查在本文件实现

#include "./../PublicAPI.h"
#include "PluginOperate.h"
#include "Plugin.h"
#include "./../Tcp/Tcp.h"

//==================================================
//全局变量 作用域：整个程序
//==================================================
//注：因为实际插件数量大概在100左右、且最频繁操作为查询、所以采用数组结构而不是链表结构
struct PluginNode plugin_list[MAXN];//插件结构列表
int plugin_maxnum; //插件的最大数量值
int plugin_flag[MAXN];//0表示空、1表示有插件
//===========================================
//引用全局变量    作用域：本文件
//===========================================
extern pthread_mutex_t plugin_pmt[MAXN];//防止同一时间重复调用到相同的脚本插件产生冲突

//==================================================
//静态全局变量  作用域：本文件
//==================================================
static int plugin_counts; //插件的实际数量 如果实际插件数量达到900则触发警报、提醒清除无用的插件
//状态标记、数组存储


//==================================================
//内部函数声明 作用域：本文件
//==================================================

int Plugin_ModCheck(char* name, int rate, int flag, int type, int direction,  \
                int system, char* version, int run_times,   \
                char* parameter, char* content);



void Plugin_DataInit()
{
	memset(plugin_flag, 0, sizeof(plugin_flag));
	plugin_maxnum = 0;//插件列表初始化为空
}


//===========================================
//              增加模块
//===========================================

//0表示增加失败   1表示增加成功
int Plugin_AddNode(char* name, int rate, int flag, int type, int direction,  \
                int system, char* version, int run_times,   \
                char* parameter, char* content)
{
    //检查数据是否合法
    int res = Plugin_ModCheck(name, rate, flag, type, direction, system, version, run_times, parameter, content);
    if(res != 0) return 0;

    char inibuf[1024] = {0};
    char buf[1024] = {0};
    int maxnum = Plugin_Get_maxnum();
	int i;
    for(i = 0; i <= maxnum; i++) //有可能是加到最后一个所以要<=
    {
        if(plugin_flag[i] == 0)
        {
            strcpy(plugin_list[i].name, name);
            plugin_list[i].rate = rate;
            plugin_list[i].flag = flag;
            plugin_list[i].type = type;
            plugin_list[i].direction = direction;
            plugin_list[i].system = system;
            strcpy(plugin_list[i].version, version);
            plugin_list[i].run_times = run_times;
            strcpy(plugin_list[i].parameter, parameter);
            strcpy(plugin_list[i].content, content);
            //写入配置文件和源代码文件
            sprintf(inibuf, "%s.ini", name);
            if(type == 0) sprintf("%s.py", name);
            else if(type == 1) sprintf("%s.sh", name);
            else return 0; //未找到对应的类型
            FILE *fp;
            fp = fopen(inibuf, "w+");
            fprintf(fp, "%s %d %d %d %d %d %s %d %s %s\n",    \
                plugin_list[i].name,     \
                plugin_list[i].rate,   \
                plugin_list[i].flag,   \
                plugin_list[i].type,   \
                plugin_list[i].direction,  \
                plugin_list[i].system, \
                plugin_list[i].version,    \
                plugin_list[i].run_times,  \
                plugin_list[i].path,    \
                plugin_list[i].parameter);
            fclose(fp);
            fp = NULL;
            fp = fopen(buf, "w+");
            fprintf(fp, "%s", plugin_list[i].content);
            fclose(fp);
            fp = NULL;
            plugin_flag[i] = 1;
            if(i == maxnum) maxnum++; //在末尾增加一个下标也要增加
            return 1; //增加成功
        }
    }
    return 0; //增加失败
}


//===========================================
//              删除模块
//===========================================
//  1表示删除成功   -1表示删除失败  0表示未找到脚本
int Plugin_DelNode(char* name)
{
    int i;
    int maxnum = Plugin_Get_maxnum();
    char buf[1024] = {0};
    char inibuf[1024] = {0};
    for(i = 0; i < maxnum; i++)
    {
        //寻找比对为name的插件
        if(plugin_flag[i] == 1 && (strcmp(plugin_list[i].name, name) == 0))
        {
            if(plugin_list[i].type == 0)
                sprintf(buf, "%s.py", name);
            else if(plugin_list[i].type == 1)
                sprintf(buf, "%s.sh", name);
            else return -1;
            sprintf(inibuf, "%s.ini", name);
            pthread_mutex_lock(&plugin_pmt[i]);//插件锁
            int res = remove(inibuf);
            if(res != 0) return -1; //删除失败
            res = remove(buf);
            pthread_mutex_unlock(&plugin_pmt[i]);
            if(res != 0) return -1; //删除失败
            plugin_flag[i] = 0; //标记为0表示删除
            return 1;
        }
    }
    return 0;
}


//===========================================
//              修改模块
//===========================================

//1--修改成功 0--修改失败 -1--未找到插件
int Plugin_Modify(char* name, int rate, int flag, int type, int direction,  \
                int system, char* version, int run_times,   \
                char* parameter, char* content)
{
    //检查数据是否合法
    int res = Plugin_ModCheck(name, rate, flag, type, direction, system, version, run_times, parameter, content);
    if(res != 0) return 0;
    
    int p = Plugin_Get_id(name);
    if(p == -1)//未找到对应的插件
    {
        return -1;
    }
    if(rate != -2333) plugin_list[p].rate = rate;
    if(flag != -2333) plugin_list[p].flag = flag;
    if(type != -2333) plugin_list[p].direction = direction;
    if(system != -2333) plugin_list[p].system = system;
    if(version != NULL) strcpy(plugin_list[p].version, version);
    if(run_times != -2333) plugin_list[p].run_times = run_times;
    if(parameter != NULL) strcpy(plugin_list[p].parameter, parameter);
    //修改脚本源代码时需要使用脚本插件互斥锁
    pthread_mutex_lock(&plugin_pmt[p]);//插件锁
    if(content != NULL) strcpy(plugin_list[p].content, content);
    pthread_mutex_unlock(&plugin_pmt[p]);
    
    return 0;
}

//0--合法 其他--不合法
//使用二进制表示哪些合法哪些不合法  不修改的默认值 int -2333  char* NULL
int Plugin_ModCheck(char* name, int rate, int flag, int type, int direction,  \
                int system, char* version, int run_times,   \
                char* parameter, char* content)
{
    int cnt = 0;
    int len = strlen(name);
    if(len > 100 && name != NULL) cnt |= 1;
    if(rate <= 0 && rate != -2333)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    if(flag !=  0 && flag != 1 && flag != -2333)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    if(type != 0 && type != 1 && type != -2333)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    if(direction != 0 && direction != 1 && direction != -2333)
    {
        cnt = cnt << 1;
        cnt |= 1;   
    }
    if(system != 0 && system != 1 && system != -2333)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    len = strlen(version);
    if(len > 100 && version != NULL)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    if(run_times != 0 && run_times != 1 && run_times != -2333)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    len = strlen(parameter);
    if(len > 100)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    len = strlen(content);
    if(len > 4096 && content != NULL)
    {
        cnt = cnt << 1;
        cnt |= 1;
    }
    return cnt;
}

//将配置信息更新到文件中
//这里可以优化、后面来调整
void Plugin_ConfigInfo()
{
    int i;
    FILE *fp;
    for(i = 0; i < plugin_maxnum; i++)
    {
        char path[105] = {0};
        sprintf(path, "./Plugins/%s.ini", plugin_list[i].name);
		printf("path:%s\n", path);
        fp = fopen(path, "w+");
		if(fp == NULL)
		{
			printf("未找到配置文件!\n");
		}
		strcpy(plugin_list[i].parameter, "abc");
        fprintf(fp, "%s %d %d %d %d %d %s %d %s %s",    \
                plugin_list[i].name,     \
                plugin_list[i].rate,   \
                plugin_list[i].flag,   \
                plugin_list[i].type,   \
                plugin_list[i].direction,  \
                plugin_list[i].system, \
                plugin_list[i].version,    \
                plugin_list[i].run_times,  \
                plugin_list[i].path,    \
                plugin_list[i].parameter);
                /*
        printf("%s %d %d %d %d %d %s %d %s %s\n",    \
                plugin_list[i].name,     \ 
                plugin_list[i].rate,   \ 
                plugin_list[i].flag,   \
                plugin_list[i].type,   \
                plugin_list[i].direction,  \
                plugin_list[i].system, \
                plugin_list[i].version,    \
                plugin_list[i].run_times,  \
                plugin_list[i].path,    \
                plugin_list[i].parameter);
                 */
        fclose(fp);
        fp = NULL;
    }
    
}
//===========================================
//              查询模块
//===========================================

//根据脚本名称查询脚本下标 -1表示未找到
int Plugin_Get_id(char* name)
{
    int i, p = -1;
    int maxnum = Plugin_Get_maxnum();
    for(i = 0; i < maxnum; i++)
    {
        if(strcmp(name, plugin_list[i].name) == 0)
        {
            p = i;
            break;
        }
    }
    return p;
}

//获取插件实际数量
int Plugin_Get_counts()
{
    return plugin_counts;
}

//获取插件下标最大值
int Plugin_Get_maxnum()
{
	return plugin_maxnum;
}

//获取某个插件结构体数据
struct PluginNode Plugin_GetPluginNode(int p)
{
	return plugin_list[p];
}

//判断p位置是否有插件
int Plugin_GetFlag(int p)
{
    return plugin_flag[p];
}

//获取插件的开启停用状态
int Plugin_GetScriptStatus(int p)
{   
    //0表示关闭 1表示启用
    return plugin_list[p].flag;
}


//改变插件的运行频率和运行次数
void AgentPluginRateTime(char* name, int rate, int times)
{
	int i;
    for(i = 0; i < plugin_maxnum; i++)
    {
        if(strcmp(name, plugin_list[i].name) == 0)
        {
            plugin_list[i].rate = rate;
			plugin_list[i].run_times = times;
			//修改成功返回消息
			char sendbuf[4096] = {0};
			sprintf(sendbuf,"{\"cmd\":1103,\"json\":{\"script_title\":\"%s\",\"run_second\":%d,\"run_times\":%d}}",name,rate,times);
			
			Agent_Log(sendbuf);
			if(strlen(sendbuf) >= BUFSIZE)
			{
				Agent_Log("发送消息过长!");
				return;
			}
            SendMessage(sendbuf);
            return;
        }
    }
}

//脚本开启
void AgentPluginStart(char* name, int rate, int times, char* parameter, char* operate_name, int operate_imid)
{
	Agent_Log("AgentPluginStart");
	//strcpy(parameter, "abc");
	int i;
    for(i = 0; i < plugin_maxnum; i++)
    {
        if(strcmp(name, plugin_list[i].name) == 0)
        {
			plugin_list[i].flag = 1;
            plugin_list[i].rate = rate;
			plugin_list[i].run_times = times;
			//strcpy(plugin_list[i].parameter, parameter);
			Agent_Log("脚本开启修改成功");
			//修改成功返回消息
			char sendbuf[4096] = {0};
			
			sprintf(sendbuf,"{\"cmd\":1101,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":\"%s\",\"run_second\":%d,\"run_times\":%d,\"custom_parameter\":\"%s\",\"operate_name\":\"%s\",\"operate_imid\":%d}}",name,rate,times,parameter,operate_name,operate_imid);
			if(strlen(sendbuf) >= BUFSIZE)
			{
				Agent_Log("发送消息过长!");
				return;
			}
            SendMessage(sendbuf);
            return;
        }
    }
}

//脚本停用
void AgentPluginStop(char* name, char* operate_name, int operate_imid)
{
	int i;
    for(i = 0; i < plugin_maxnum; i++)
    {
        if(strcmp(name, plugin_list[i].name) == 0)
        {
			plugin_list[i].flag = 0;
			
			//修改成功返回消息
			char sendbuf[4096] = {0};
			sprintf(sendbuf,"{\"cmd\":1102,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":\"%s\",\"operate_name\":\"%s\",\"operate_imid\":%d}}",name,operate_name,operate_imid);
			if(strlen(sendbuf) >= BUFSIZE)
			{
				Agent_Log("发送消息过长!");
				return;
			}
            SendMessage(sendbuf);
            return;
        }
    }
}


//同步客户端脚本
void AgentPluginMsgToServer()
{
    int i;
    for(i = 0; i < plugin_maxnum; i++)
    {
        char sendbuf[4096] = "{\"cmd\":1108,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_list\":[";
        char tempbuf[4096] = {0};
        sprintf(tempbuf,"%s{\"version\":\"%s\",\"script_title\":\"%s\",\"content\":\"%s\",\"type\":%d,\"source_type\":%d}",tempbuf,plugin_list[i].version,plugin_list[i].name,plugin_list[i].content,plugin_list[i].type,1);
        sprintf(sendbuf, "%s%s]}}", sendbuf, tempbuf);
		Agent_Log(sendbuf);
		if(strlen(sendbuf) >= BUFSIZE)
		{
			Agent_Log("发送消息过长!");
			return;
		}
        SendMessage(sendbuf);
    }
    return;
}

void AgentPluginSetRunonce(int p, int flag)
{
    plugin_list[p].runonce = flag;
    return;
}

void AgentPluginRunOnce(char* name, char* operate_name, char* operate_imid)
{
    char llog[1024] = {0};
    sprintf(llog, "runonce:%s %s %d", name, operate_name, operate_imid);
    Agent_Log(llog);
    int i;
    for(i = 0; i < plugin_maxnum; i++)
    {
        if(strcmp(name, plugin_list[i].name) == 0)
        {
            plugin_list[i].runonce = 1;

            char parameter[105] = {0};
            strcpy(parameter, plugin_list[i].parameter);

            Agent_Log("脚本临时执行");
            //修改成功返回消息
            char sendbuf[4096] = {0};

            sprintf(sendbuf,"{\"cmd\":1101,\"token\":\"n7sbl64qb88l0xy\",\"json\":{\"script_title\":\"%s\",\"run_second\":%d,\"run_times\":%d,\"custom_parameter\":\"%s\",\"operate_name\":\"%s\",\"operate_imid\":%d}}",name,1,1,parameter,operate_name,operate_imid);
            if(strlen(sendbuf) >= BUFSIZE)
            {
                Agent_Log("发送消息过长!");
                return;
            }
            Agent_Log(sendbuf);
            SendMessage(sendbuf);
            return;
        }
    }
}