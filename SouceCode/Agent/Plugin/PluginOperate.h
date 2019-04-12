#ifndef _PLUGINOPERATE_H
#define _PLUGINOPERATE_H


//==================================================
//数据定义
//==================================================
//插件结构体、关键数据结构
//尽可能优化效率、
struct PluginNode{

	char name[105]; //插件名称
	
	int rate; //插件运行的时间频率
	
	int flag; //插件是否启用、0表示关闭 1表示启用

	int type; //0表示python脚本、1表示shell脚本、

	int direction; //0表示发往大数据、1表示发往服务端

	int system; //0表示普通服务器、1表示游戏服务器
	
	char version[105];//版本号

	int run_times;//运行次数

	char path[105]; //插件路径  plugins/Yunwei/cpu.py

	char parameter[105]; //传入参数
	
	char content[4096]; //脚本源码
	
	char result[4096]; //脚本执行结果

	//int root; //插件的运行权限、1表示root权限、0表示非root权限

	int runonce;//1表示临时执行0表示默认状态不执行
};


//==================================================
//对外函数借口
//==================================================
int Plugin_AddNode(char* name, int rate, int flag, int type, int direction,  \
                int system, char* version, int run_times,   \
                char* parameter, char* content);

int Plugin_DelNode(char* name);

int Plugin_Modify(char* name, int rate, int flag, int type, int direction,  \
                int system, char* version, int run_times,   \
                char* parameter, char* content);

void Plugin_ConfigInfo();

int Plugin_Get_id(char* name);

int Plugin_Get_counts();

int Plugin_Get_maxnum();

struct PluginNode Plugin_GetPluginNode(int p);

int Plugin_GetFlag(int p);

int Plugin_GetScriptStatus(int p);

void AgentPluginRateTime(char* name, int rate, int times);

void AgentPluginStart(char* name, int rate, int times, char* parameter, char* operate_name, int operate_imid);

void AgentPluginStop(char* name, char* operate_name, int operate_imid);

void AgentPluginMsgToServer();

void Plugin_DataInit();

void AgentPluginRunOnce(char* name, char* operate_name, char* operate_imid);

void AgentPluginSetRunOnce(int p, int flag);

#endif
