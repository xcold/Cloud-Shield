#ifndef _AGENTMANAGE_H
#define _AGENTMANAGE_H


//==================================================
//数据定义
//==================================================
//agent健康状态结构体
struct AgentNode
{
	int pid; //agent进程ID
	float machine_cpu; //agent所在机器的cpu的使用率
	float machine_memory; //agent所在机器的内存使用率
	float agent_cpu; //agent占用的cpu
	float agent_memory; //agent占用的内存
	float security_cpu; //安全系统模块占用cpu
	float plugin_cpu; //插件管理模块占用cpu

	//安全状态的标记、通知服务端来获取具体的安全信息
	//0表示正常、1表示异常
	int process; //是否发现异常进程
	int port; //是否发现异常端口
	int login; //是否发现异常登录
	int account; //是否发现异常账户

	int webshell; //是否检测到异常的web文件
	int ddos; //是否检测到异常的流量攻击
};

//agent线程信息结构体
struct ThreadInfo
{
	int tid;//线程ID
	float cpu;//cpu占用百分比
};


//==================================================
//对外函数借口
//==================================================
void Plugin_Manage();

void Security_Manage();

void AgentManage_Running();

#endif
