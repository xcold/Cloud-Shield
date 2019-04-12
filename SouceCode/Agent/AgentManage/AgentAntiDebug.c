//C语言实现Agent反调试检测模块
//增加自我保护检测、反调试检测、防止调试器附加和恶意线程进行注入
//为防止反汇编静态分析被找到检测代码篡改从而绕过反调试检测
//我们还需要对二进制文件进行混淆以对抗静态分析

//包含头文件
#include "./../PublicAPI.h"
#include "AgentManage.h"
#include <sys/ptrace.h>

/*

//判断自身是否可被调试来判断是否已被其他调试器附加
//返回1表示检测到异常、返回0表示正常
int AgentAntiDebug_Ptrace()
{
	if(ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
		return 1;
	else
		return 0;
}

//检查父进程的名称
//返回1表示检测到异常、返回0表示正常
int AgentAntiDebug_Ppid()
{
	char path[32], name[128];
	FILE *fp;
	snprintf(path, 24, "/proc/%d/cmdline", getpid());
	fp = fopen(path, "r");
	if(fp == NULL) return 0;
	fgets(name, 128, fp);
	fclose(fp);

	if(strcmp(name, "gdb") == 0) return 1;
	else return 0;
}

//检查进程运行状态
//返回1表示检测到异常、返回0表示正常
int AgentAntiDebug_RunStatus()
{
	char buf[512];
	FILE *fp;
	fp = fopen("/proc/self/status", "r");
	if(fp == NULL) return 0;
	int tpid;
	const char *needle = "TracerPid:";
	size_t nl = strlen(needle);
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		if(!strncmp(buf, needle, nl))
		{
			sscanf(buf, "TracerPid: %d", &tpid);
			if(tpid != 0) return 1;
		}
	}
	return 0;
}

//检测自身线程数量是否超过最大值
int AgentAntiDebug_ThreadNum()
{
	//查询当前某程序的线程或进程数
	//pstree -p `ps -e | grep java | awk '{print $1}'` | wc -l
	return 0;
}


*/