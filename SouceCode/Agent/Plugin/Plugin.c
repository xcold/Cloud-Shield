//==================================================
//插件管理系统模块
//==================================================


#include "./../PublicAPI.h"
#include "Plugin.h"
#include "./../Log/Log.h"
#include "./../Http/AgentHttp.h"
#include "./../cJSON/cJSON.h"
#include "PluginOperate.h"
#include "./../ThreadPool/ThreadPool.h"
#include "PluginSendData.h"
#include "./../Tcp/Tcp.h"

//==================================================
//数据定义  作用域：本文件
//==================================================


//==================================================
//引用全局变量  作用域：本文件
//==================================================
extern struct PluginNode plugin_list[MAXN];//插件结构列表
extern int plugin_maxnum;
extern int plugin_flag[MAXN];
//==================================================
//全局变量  作用域：整个程序
//==================================================
//插件互斥锁
pthread_mutex_t plugin_pmt[MAXN];//防止同一时间重复调用到相同的脚本插件产生冲突

//==================================================
//静态全局变量  作用域：本文件
//==================================================
//插件管理系统挂起与恢复相关数据
static pthread_mutex_t plugin_manage_pmt = PTHREAD_MUTEX_INITIALIZER;//插件管理系统线程互斥变量  
static pthread_cond_t plugin_manage_cond = PTHREAD_COND_INITIALIZER;  
static int plugin_manage_status = RUN;
static int plugin_manage_flag = 1;//插件管理系统是否运行标志 0表示挂起、1表示运行






//插件管理系统线程数据
static int sec_count = 0; //时间统计
static struct threadpool *pool; //线程池

//==================================================
//对内声明函数
//==================================================
static void Plugin_GetScript(char *basePath);

static void Plugin_Filter();

static void* Plugin_Running(void* arg);

static void PluginThread_Init();

static void PluginThread_Pause();

static void PluginThread_Resume();

//----------数据定义------------start
//监控信息脚本返回格式如下
//[(metric1, value1, tag), (metric2, value2, tag)...]
//----------数据定义------------end


//插件管理系统初始化
//初始化插件列表数据、先读取xx.ini配置文件、如果没有配置文件则
int Plugin_Init()
{
    //初始化插件锁
    int i;
    for(i = 0; i < MAXN; i++)
    {
        pthread_mutex_init(&plugin_pmt[i], NULL);
    }
    //初始化线程锁
    PluginThread_Init();

    //初始化线程池
    pool = threadpool_init(10,20); //10个线程、队列最大20

	
	Plugin_DataInit();
	//printf("aaa23333\n");
	//获取plugins目录下的脚本
	char basePath[BUFSIZE] = {0};//插件脚本路径
	memset(basePath,'\0',sizeof(basePath));
    strcpy(basePath,"./Plugins/");
    Plugin_GetScript(basePath);
	//printf("23333\n");
	return 0;
}

//自动读取Plugins/Yunwei目录和Plugins/Service下的脚本
void Plugin_GetScript(char *basePath)
{
	struct dirent *ptr;
	DIR *dir;
	
    //得到当前的绝对路径
    //memset(basePath,'\0',sizeof(basePath));
    //getcwd(basePath, BUFSIZE - 1);
	
	if((dir = opendir(basePath)) == NULL)
	{
		char log[1024] = {0};
        sprintf(log, "打开%s目录失败", basePath);
		Agent_Log(log);
		return;
	}
	printf("success\n");
	FILE *fp;
	for(ptr = readdir(dir); NULL != ptr; ptr = readdir(dir))
	{
        //判断是否有配置文件、如果没有配置文件就不认可
		if(strstr(ptr->d_name, ".ini") != NULL)
		{
			char fullpath[1024] = {0};
			if(strlen(basePath) + strlen(ptr->d_name) >= sizeof(fullpath))
			{
				char log[1024] = {0};
            	sprintf(log, "插件配置文件路径长度错误:%s", ptr->d_name);
				Agent_Log(log);
			}
			sprintf(fullpath, "%s%s", basePath, ptr->d_name);
			int len_path = strlen(fullpath);
			//有这个py或sh脚本
			printf("%s\n", ptr->d_name);
		
			//读取脚本配置信息
			fp = fopen(fullpath, "rb+");
            if(fp == NULL)
            {
            	char log[1024] = {0};
            	sprintf(log, "无法打开脚本配置文件:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            //临时变量
            char name[BUFSIZE] = {0};
            int rate = -2333;
            int flag = -2333;
            int type = -2333;
            int direction = -2333;
            int system = -2333;
            char version[BUFSIZE] = {0};
            int run_times = 0;
            char path[BUFSIZE] = {0};
            char parameter[BUFSIZE] = {0};

            fscanf(fp, "%s%d%d%d%d%d%s%d%s%s", name, &rate, &flag, &type, &direction,	\
            				 				&system, version, &run_times, path, parameter);

            fclose(fp);
            fp = NULL;

            //校验数据合法性
            if(strlen(name) > 100 || strlen(name) <= 0)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件名称不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(rate <= 0 || rate == -2333)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件运行频率不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
			printf("flag: %d\n", flag);
            if(flag != 0 && flag != 1)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件开关标记不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(type != 0 && type != 1)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件type字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(direction != 0 && direction != 1)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件direction字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(system != 0 && system != 1)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件system字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(strlen(version) > 100 || strlen(version) <= 0)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件version字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(run_times < -1 || run_times == -2333)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件run_times字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(strlen(path) > 100 || strlen(path) <= 0)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件path字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }
            if(strlen(parameter) > 100)
            {
            	char log[1024] = {0};
            	sprintf(log, "配置文件parameter字段不合法:%s", fullpath);
            	Agent_Log(log);
                continue; //一个插件无法读取不影响整个程序的运行
            }

            strcpy(plugin_list[plugin_maxnum].name, name); 
            plugin_list[plugin_maxnum].rate = rate;
            plugin_list[plugin_maxnum].flag = flag;
            plugin_list[plugin_maxnum].type = type;
            plugin_list[plugin_maxnum].direction = direction;
            plugin_list[plugin_maxnum].system = system;
            strcpy(plugin_list[plugin_maxnum].version, version);
            plugin_list[plugin_maxnum].run_times = run_times;
            strcpy(plugin_list[plugin_maxnum].path, path);
			strcpy(plugin_list[plugin_maxnum].parameter, parameter);
			

			
			char script_path[1024] = {0};
			int i;
			for(i = 0; i < len_path - 3; i++)
			{
				script_path[i] = fullpath[i]; 
			}
			if(type == 0)//源文件类型是python文件
			{
				script_path[len_path - 3] = 'p';
				script_path[len_path - 2] = 'y';
				script_path[len_path - 1] = '\0';
			}
			else//源文件类型是shell文件
			{
				script_path[len_path - 3] = 's';
				script_path[len_path - 2] = 'h';
				script_path[len_path - 1] = '\0';
			}
			char content[BUFSIZE] = {0};
			char full_content[BUFSIZE]= {0};
			if((fp = fopen(script_path, "r")) == NULL)
            {
            	char log[1024] = {0};
            	sprintf(log, "无法打开脚本文件:%s", script_path);
            	Agent_Log(log);
                continue;
            }
			else
			{
				while(fgets(content,sizeof(content),fp) != NULL)
				{
					sprintf(full_content, "%s%s", full_content, content);
				}

				fclose(fp);
	            fp = NULL;

				int len_content = strlen(full_content);
				if(len_content <= 0)
				{
					char log[1024] = {0};
	            	sprintf(log, "脚本文件内容不合法:%s", script_path);
	            	Agent_Log(log);
	                continue;
				}
				//如果脚本文件内容长度过多、则只同步前面部分给云盾服务端
				int j;
				int cnt_id = 0;
				for(j = 0; j < len_content; j++)
				{
					if(full_content[j] == '\"')
					{
						plugin_list[plugin_maxnum].content[cnt_id++] = '\\';
						plugin_list[plugin_maxnum].content[cnt_id++] = '\"';
					}
					else if(full_content[j] == '\'')
					{
						plugin_list[plugin_maxnum].content[cnt_id++] = '\\';
						plugin_list[plugin_maxnum].content[cnt_id++] = '\'';
					}
					else if(full_content[j] == '\\')
					{
						plugin_list[plugin_maxnum].content[cnt_id++] = '\\';
						plugin_list[plugin_maxnum].content[cnt_id++] = '\\';
					}
					else
					{
						plugin_list[plugin_maxnum].content[cnt_id++] = full_content[j];
					}
				}
				plugin_list[plugin_maxnum].content[cnt_id] = '\0';
				plugin_flag[plugin_maxnum] = 1;
			}
			plugin_maxnum++;
		}
	}
	int i;
	for(i = 0; i < plugin_maxnum; i++)
	{
		printf("%s %d %d %d %d %d %s %d %s %s\n", plugin_list[i].name, plugin_list[i].rate, plugin_list[i].flag, plugin_list[i].type, plugin_list[i].direction,	\
            				 				plugin_list[i].system, plugin_list[i].version, plugin_list[i].run_times, plugin_list[i].path, plugin_list[i].parameter);
		//printf("%s", plugin_list[i].content);
	}
	
	
	closedir(dir);
	printf("xxx\n");
}

// 插件调度管理
void Plugin_Filter()
{
    //互斥体判断是否运行程序
    pthread_mutex_lock(&plugin_manage_pmt);   
    while(plugin_manage_flag == STOP)
    {   
        pthread_cond_wait(&plugin_manage_cond, &plugin_manage_pmt);   
    }   
    pthread_mutex_unlock(&plugin_manage_pmt);
    
	//printf("working...............................\n");
    //working
    //判断当前的seconds
    int i;
    int arr[105] = {0};
    //遍历插件列表
	int maxnum = Plugin_Get_maxnum();
	
    for(i = 0; i < maxnum; i++)
    {
		//printf("i............%d\n", i);
		struct PluginNode plugin = Plugin_GetPluginNode(i);
		//printf("plugin....%d %d %d\n", plugin.flag, plugin.rate, plugin.run_times);
        //判断插件存在&&启用&&插件频率符合
		int flag = Plugin_GetFlag(i);
		//printf("flag................:%d\n", flag);
        if(flag == 0) continue; //这个位置没有插件
        if(plugin.rate == 0) continue; //模数不能为0
        if(plugin.runonce == 1)//临时执行脚本一次
        {
        	arr[i % 100] = i;
        	threadpool_add_job(pool, (void *)Plugin_Running, (void *)&arr[i]);
        	//设置runonce为0
        	AgentPluginSetRunonce(arr[i % 100], 0);
        }
        if(plugin.flag == 1 && ((sec_count % plugin.rate) == 0) && (plugin.run_times > 0 || plugin.run_times == -1))//插件开启
        {
			//printf("running...............................running...\n");
            arr[i % 100] = i; //这里保证线程池队列最大不超过100
            threadpool_add_job(pool, (void *)Plugin_Running, (void *)&arr[i]);
        }
    }
    sec_count++;//计时
	printf("sec_count:%d\n", sec_count);
    if(sec_count % 10 == 0)
    {
        Plugin_ConfigInfo();
        printf("脚本数据写入配置文件......\n");
        //exit(0);
    }
}

//管理插件分配时间运行、加入线程池
//在线程用定时器的原因是因为定时器优先级高、处理可能卡顿
void Plugin_Manage()
{
    //使用这种定时器方法可以避免信号量触发sleep、pause
    //struct itimerval tick;
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;

    memset(&evp, 0, sizeof(evp));
    evp.sigev_value.sival_ptr = &timer;
    evp.sigev_value.sival_int = 3;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = Plugin_Filter;
    
    int ret = timer_create(CLOCK_REALTIME, &evp, &timer);
    if(ret < 0)
    {
        Agent_Log("createtimer failed!");
        exit(EXIT_FAILURE); //插件管理模块启动失败退出Agent重启
        return;
    }

    //每次更新时间
    ts.it_interval.tv_sec = 1;
    ts.it_interval.tv_nsec = 0;
    //首次设定时间
    ts.it_value.tv_sec = 3;
    ts.it_value.tv_nsec = 0;
    //sec_count = GetTimeSecond();
    sec_count = 1;//用变量计时消除误差
    ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL);
    if(ret < 0)
    {
        Agent_Log("setitimer failed!");
        exit(EXIT_FAILURE); //插件管理模块启动失败退出Agent重启
        return;
    }
}



//插件脚本运行模块
//运行python脚本插件 线程池调用的函数不能用全局变量
void* Plugin_Running(void* arg)
{
	int p = *(int *)arg;;//无值型指针需要先指明类型
	pthread_mutex_lock(&plugin_pmt[p]);//插件锁
	if(plugin_list[p].runonce == 0)
	{
		if(plugin_list[p].run_times > 0)plugin_list[p].run_times--;//执行一次减一次
	}
	//Agent_Log("plugin running....\n");
	char str_name[1024] = {0};
	char temp[1024] = {0};
	char str[1024] = {0};
	//遍历python脚本文件名
	char baseFile[1024] = {0};
	strcpy(baseFile, "./../Plugins/");
	sprintf(str_name, "%s", plugin_list[p].name);
	sprintf(temp, "%s.py", plugin_list[p].name);
	strcat(baseFile, temp);
	printf("basePath %s\n", baseFile);
	if(access(baseFile, F_OK) != 0){
		char log[1024];
		sprintf("no file %s\n", baseFile);
		Agent_Log(log);
		return NULL; //没有这个脚本文件
	}
	//sprintf(str, "python %s > ./../Resource/%s.txt", baseFile, str_name);
    sprintf(str, "python %s", baseFile);
    
	//chmod u+x file.sh 对当前目录下的file.sh文件的所有者增加可执行权限
	//chmod u+x 设定为只有该档案拥有者可以执行
	//注意事项：在编写具 SUID/SGID 权限的程序时请尽量避免使用popen()、popen()会继承环境变量
	//通过环境变量可能会造成系统安全的问题。
	//char cmd[1024] = {0};
	//sprintf(cmd, "chmod u+x %s", str_name);
	//int res = my_system(cmd);
    char result[BUFSIZE] = {0};
	int res = my_system(str, result);//使用自定义封装API

    if(res != 0) //python脚本运行出错
	{
        printf("%d.py python shell error!\n", p);
        //AddLog("")
	}
	int len_result = strlen(result);
	if(len_result > 3500)
	{
		memset(result, 0, sizeof(result));
		strcpy(result, "返回结果太长，无法显示");
	}
	int j;
	int cnt_id = 0;
	for(j = 0; j < len_result; j++)
	{
		if(result[j] == '\"')
		{
			plugin_list[p].result[cnt_id++] = '\\';
			plugin_list[p].result[cnt_id++] = '\"';
		}
		else if(result[j] == '\'')
		{
			plugin_list[p].result[cnt_id++] = '\\';
			plugin_list[p].result[cnt_id++] = '\'';
		}
		else if(result[j] == '\\')
		{
			plugin_list[p].result[cnt_id++] = '\\';
			plugin_list[p].result[cnt_id++] = '\\';
		}
		else
		{
			plugin_list[p].result[cnt_id++] = result[j];
		}
	}
	plugin_list[p].result[cnt_id] = '\0';
	
    //strcpy(plugin_list[p].result, result);
    
    Plugin_ResSend(p);

	pthread_mutex_unlock(&plugin_pmt[p]);
	return NULL;
}

//初始化线程锁
void PluginThread_Init()
{   
    pthread_mutex_init(&plugin_manage_pmt,NULL);  
    pthread_cond_init(&plugin_manage_cond,NULL); 
}   

//停止运行插件管理系统
void PluginThread_Pause()
{
	if(plugin_manage_status == RUN)
	{
	    pthread_mutex_lock(&plugin_manage_pmt);   
	    plugin_manage_status = STOP;
	    //插件管理系统关闭成功
	    char msg[1024] = "{\"cmd\":1000,\"code\":1,\"message\":\"插件管理系统关闭成功\"}";
	    SendMessage(msg);
	    Agent_Log("插件管理系统关闭成功");
	    pthread_mutex_unlock(&plugin_manage_pmt);
	}
	else
	{
		Agent_Log("请勿重复关闭,插件管理系统已经关闭");
		char msg[1024] = "{\"cmd\":1000,\"code\":0,\"message\":\"请勿重复关闭插件管理系统\"}";
		SendMessage(msg);
	}
}   

//恢复运行插件管理系统
void PluginThread_Resume()
{
	if(plugin_manage_status == STOP)
	{
	    pthread_mutex_lock(&plugin_manage_pmt);   
	    plugin_manage_status = RUN;
	    pthread_cond_signal(&plugin_manage_cond);
	    //插件管理系统开启成功
	    char msg[1024] = "{\"cmd\":1000,\"code\":1,\"message\":\"插件管理系统开启成功\"}";
	    SendMessage(msg);
	    Agent_Log("插件管理系统开启成功");
	    pthread_mutex_unlock(&plugin_manage_pmt);
	}
	else
	{
		Agent_Log("请勿重复开启,插件管理系统已经运行");
		char msg[1024] = "{\"cmd\":1000,\"code\":0,\"message\":\"请勿重复开启插件管理系统\"}";
		SendMessage(msg);
	}
}

int Plugin_GetStatus()
{
    return plugin_manage_status;
}

//设置插件管理系统的开关状态 
void Plugin_SetStatus(int status)
{
    if(status == RUN) //开启
    {
        PluginThread_Resume();
    }
    else //关闭
    {
        PluginThread_Pause();
    }
}