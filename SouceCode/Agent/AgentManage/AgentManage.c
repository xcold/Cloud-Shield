//C语言实现Agent自我管理模块
//检测Agent自身的状态信息、以及对插件管理系统和安全管理系统挂起或运行
//实时检查是否有内存泄漏的情况、hook内存管理函数
//反调试模块开一个线程进行循环检测、主线程检测这个线程是否存活或挂起
//防止同一台服务器上Agent模块多开

//包含头文件
#include "./../PublicAPI.h"
#include "AgentManage.h"
#include "./../Log/Log.h"
#include <sys/syscall.h>  

//==================================================
//全局变量	作用域：整个程序
//==================================================
struct AgentNode AgentHealthStatus; //健康状态实例
struct ThreadInfo thread_info[1005];
//==================================================
//静态全局变量	作用域：本文件
//==================================================





static struct ThreadInfo Agent_Thread_Info[105];//存储线程的CPU、内存占用率等信息
static int Agent_Thread_Num = 0; //Agent线程的数量


//==================================================
//内部声明函数	作用域：本文件
//==================================================
static void UpdateProcessInfo();

static void UpdateThreadInfo();

static void UpdateMachineInfo();

pid_t gettid(void);

pid_t gettid(void)
{  
	return syscall(SYS_gettid);  
}

//主线程
//对各个管理系统运行状态进行管理  线程挂起或恢复运行
void AgentManage_Running()
{
	//printf("AgentManage .........\n");
	//初始化状态结构体
	memset(&AgentHealthStatus, 0, sizeof(AgentHealthStatus));
	//初始化线程信息结构体
	memset(&Agent_Thread_Info, 0, sizeof(Agent_Thread_Info));

	int tid = gettid();
	//printf("tid ........%d\n", tid);
	
	int writenum = 1;
	
	while(1)
	{
		//FILE *fp;
		//fp = fopen("DaemonToAgent.txt", "w");
		//fprintf(fp, "%d", writenum);
		//writenum++;
		//fflush(fp);
		//close(fp);
		//fp = 0;
		
		//更新进程的信息保存在内存中
		UpdateProcessInfo();
		
		//更新线程的信息保存在内存中
		UpdateThreadInfo();

		//获取当前机器的CPU占用率和内存占用率
		UpdateMachineInfo();

		//判断当前进程的CPU和内存占用是否超过阀值---默认超过10%进行报警---超过20%直接关闭
		if(AgentHealthStatus.agent_cpu > 10 || AgentHealthStatus.agent_memory > 10)
		{
			//进程CPU或内存超过阀值则判断为异常
			//如果程序异常则释放资源并重启程序
			if(AgentHealthStatus.agent_cpu > 20 || AgentHealthStatus.agent_memory > 20)
			{
				Agent_Log("Agent CPU或内存占用率超过20%%,重启Agent!\n");
				//exit(1);
			}
			//向服务端发送警报消息

		}
		sleep(4);
	}
}

void UpdateMachineInfo()
{
	//printf("updatemachineinfo.......\n");
	float usercpu = 0;
	float systemcpu = 0;
	//float memory = 0;
	int total = 0;
	int used = 0;
	int free = 0;

	char result[1024] = {0};
	//获取用户CPU占用率	 top -n 1 |grep Cpu | cut -d "," -f 1 | cut -d ":" -f 2
	//  1.9%us
	//int res = my_system("top -n 1 |grep Cpu | cut -d \",\" -f 1 | cut -d \":\" -f 2", result);
	//printf("machineinfo:%s\n", result);
	//int len = strlen(result);
	//result[len - 3] = '\0';
	//usercpu = atof(result);
	system("top -n 1 |grep Cpu | cut -d \",\" -f 1 | cut -d \":\" -f 2 > usercpu.txt");
	FILE *fp;
	fp = fopen("usercpu.txt", "r");
	fscanf(fp, "%s%f%s",result, &usercpu, result);
	fflush(fp);
	fclose(fp);
	printf("usercpu %f\n", usercpu);
	//获取系统CPU占用率  top -n 1 |grep Cpu | cut -d "," -f 2
	//  1.3%sy
	memset(result, 0, sizeof(result));
	//res = my_system("top -n 1 |grep Cpu | cut -d \",\" -f 2", result);
	//printf("machineinfo     %s\n", result);
	//len = strlen(result);
	//result[len - 3] = '\0';
	//systemcpu = atof(result);
	system("top -n 1 |grep Cpu | cut -d \",\" -f 2 > systemcpu.txt");
	fp = fopen("systemcpu.txt", "r");
	fscanf(fp, "%s%f%s",result, &systemcpu, result);
	fflush(fp);
	fclose(fp);
	printf("systemcpu %f\n", systemcpu);
	AgentHealthStatus.machine_cpu = usercpu + systemcpu;
	printf("machinecpu %f\n", AgentHealthStatus.machine_cpu);
	//获取内存占用率  top -n 1 |grep Mem
	//Mem:   2034940k total,  1754148k used,   280792k free,   118280k buffers
	//获取内存总量  top -n 1 |grep Mem: | cut -d "," -f 1 | cut -d ":" -f 2
	//  2034940k total
	memset(result,  0, sizeof(result));
	int res = my_system("cat /proc/meminfo | grep MemTotal", result);
	//printf("MemTotal%s\n", result);
	char tmp[1024] = {0};
	int len = strlen(result);
	int i;
	for(i = 0; i < len; i++)
	{
		if(result[i] >= '0' && result[i] <= '9')
		{
			total = total * 10 + result[i] - '0';
		}
	}
	//printf("total %d\n", total);
	//int len = strlen(result);
	//result[len - 7] = '\0';
	//total = atoi(result);
	//获取内存已使用量  top -n 1 |grep Mem: | cut -d "," -f 2
	//  1753984k used
	
	memset(result, 0, sizeof(result));
	res = my_system("cat /proc/meminfo | grep MemFree", result);
	//printf("MemTotal%s\n", result);
	memset(tmp, 0, sizeof(tmp));
	len = strlen(result);
	for(i = 0; i < len; i++)
	{
		if(result[i] >= '0' && result[i] <= '9')
		{
			free = free * 10 + result[i] - '0';
		}
	}
	//printf("free %d\n", free);
	used = total - free;
	//res = my_system("top -n 1 |grep Mem: | cut -d \",\" -f 2", result);
	//printf("machineinfo     %s\n", result);
	//len = strlen(result);
	//result[len - 6] = '\0';
	//used = atoi(result);
	if(total != 0) //除数不能为0
	AgentHealthStatus.machine_memory = (float)used / (float)total;
	AgentHealthStatus.machine_memory = AgentHealthStatus.machine_memory * 100;
	//printf("%f\n", AgentHealthStatus.machine_memory);
}

void UpdateProcessInfo()
{
	//printf("updateprocessinfo.......\n");
	int pid = getpid(); //getpid()获取当前进程的pid
	//获取进程的CPU、内存状态
	//ps aux | awk '{print($2" "$3" "$4)}'|grep pid
	//pid cpu mem     68159 0.0 0.3
	char GetProcessCmd[105] = {0};
	sprintf(GetProcessCmd ,"ps aux | awk \'{print($2\" \"$3\" \"$4)}\'|grep %d", pid);
	FILE *Process_fp;
	int res = 0;

	if((Process_fp = popen(GetProcessCmd, "r")) == NULL)
	{
		char log[205] = {0};
		sprintf(log, "读取进程状态---popen函数执行出错:%s\n", strerror(errno));
		Agent_Log(log);
		exit(1);
	}
	else
	{
		//fread(&AgentHealthStatus, sizeof(AgentHealthStatus), 1, Process_fp);
		fscanf(Process_fp, "%d%f%f", &AgentHealthStatus.pid, &AgentHealthStatus.agent_cpu, &AgentHealthStatus.agent_memory);
		if((res = pclose(Process_fp)) == -1)
		{
			Agent_Log("读取进程状态---popen关闭函数出错!\n");
			exit(1);
		}
	}
	printf("AgentHealthStatus %d %f %f\n", AgentHealthStatus.pid, AgentHealthStatus.agent_cpu, AgentHealthStatus.agent_memory);
}

//从文件中读取各个线程的PID/CUP的数值、内存是共享的
void UpdateThreadInfo()
{
	printf("updatethreadinfo.......\n");
	//获取当前各线程的CPU占用情况		
	//ps mp pid -o %cpu,tid
	int pid = getpid();
	char thread_str[105] = {0};
	sprintf(thread_str, "ps mp %d -o %%cpu,tid", pid);
	FILE *Thread_fp;
	int res = 0;
	
	if((Thread_fp = popen(thread_str, "r")) == NULL)
	{
		char log[205] = {0};
		sprintf(log, "读取线程状态---popen函数执行出错:%s\n", strerror(errno));
		Agent_Log(log);
		exit(1);
	}
	else
	{
		char temp[1024];
		fgets(temp, sizeof(temp), Thread_fp);
		fgets(temp, sizeof(temp), Thread_fp);
		Agent_Thread_Num = 0; //前两行是没用的数据
		while(fscanf(Thread_fp, "%f %d", &thread_info[Agent_Thread_Num].cpu, &thread_info[Agent_Thread_Num].tid) != EOF)
		{
			Agent_Thread_Num++;
		}
		if((res = pclose(Thread_fp)) == -1)
		{
			Agent_Log("读取线程状态---popen关闭函数出错!\n");
			exit(1);
		}
	}
	//int i;
	//for(i = 0; i < Agent_Thread_Num; i++)
	//printf("threainfo %f %d\n", thread_info[i].cpu, thread_info[i].tid);
}
