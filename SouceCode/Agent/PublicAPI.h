#ifndef _PUBLICAPI_H
#define _PUBLICAPI_H

//项目所有的函数命名都以当前文件名称开头、以免混淆
//所有全局变量都对应一个互斥锁、让多线程同步
//所有的文件都对应一个互斥锁、防止访问冲突


//C语言库函数
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

//POSIX标准
#include <unistd.h>

//linux系统函数
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/vfs.h>
#include <sys/select.h>
#include <sys/param.h>


#define BYTE unsigned char
#define BUFSIZE 4096
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MAXN 1005
#define RUN 1
#define STOP 0


//Agent日志函数贯穿所有文件
void Agent_Log(char *buf);
//执行命令
int my_system(char *cmd, char *result);


  


#endif