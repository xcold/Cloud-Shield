//DDOS攻击检测模块

//1、流量变化	2、抓包分析

//检测原理、监测流量异常变化然后开始抓包、对来源IP和数据类型进行分析

//使用libpcap库抓包----在千兆以上网卡存在丢包现象
//优化底层缓冲区能解决丢包问题但是会导致CPU占用率提高
//这里不需要完全抓到所有包、在不影响CPU占用率的情况对大部分包的来源IP数量合理性进行分析
//对于NTP反射放大攻击、对包的类型进行统计、在规定时间内某类特殊的包数量超过一个阈值则触发警报

//ifconfig eth0 | grep bytes
//          RX bytes:92205955 (87.9 MiB)  TX bytes:15204326 (14.4 MiB)


void Security_Ddos()
{
	char result[BUFSIZE] = {0};
	int res = my_system("ifconfig eth0 | grep bytes", result);
	int len = strlen(result);
	int recvbytes = 0;
	int sendbytes = 0;
	int i;
	int flag = 0;
	for(i = 0; i < len; i++)
	{
		if(result[i] == ':')
		{
			flag++;
		}
		else if(flag == 1 && result[i] >= '0' && result[i] <= '9')
		{
			recvbytes = recvbytes * 10 + result[i] - '0';
		}
		else break;
	}
	for(i = 0; i < len; i++)
	{
		if(result[i] == ':')
		{
			flag++;
		}
		else if(flag == 2 && result[i] >= '0' && result[i] <= '9')
		{
			sendbytes = sendbytes * 10 + result[i] - '0';
		}
		else break;
	}

}


