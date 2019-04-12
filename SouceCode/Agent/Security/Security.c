//安全管理系统模块


#include "./../PublicAPI.h"

void handler(int sig)
{
	printf("catch a sig = %d\n", sig);
}

unsigned int mysleep(unsigned int secs)
{
	struct sigaction new, old;
	sigset_t newmask, oldmask, suspmask;//信号集

	unsigned int unslept = 0;
	new.sa_handler = handler;
	sigemptyset(&new.sa_mask);
	new.sa_flags = 0;
	sigaction(SIGALRM, &new, &old);//注册信号处理函数

	sigemptyset(&newmask);
	sigaddset(&newmask, SIGALRM);
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	alarm(times);//设置闹钟
	suspmask = oldmask;
	sigdelset(&suspmak, SIGALRM);
	sigsuspend(&suspmask);
	//pause();//挂起进程
	unslept = alarm(0);//清空闹钟
	sigaction(SIGALRM, &old, NULL);//NULL恢复默认信号
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
	return unslept;
}

void Security_Manage()
{
	int sec_count = 0;
	while(1)
	{
		//安全基线检测
		if(sec_count % 10 == 0)

		//木马文件扫描	
		if(sec_count % 60 == 0)
		

		//DDOS攻击检测


		my_sleep(1); //使用自定义的sleep屏蔽外来信号的影响

		sec_count++;
	}
}
