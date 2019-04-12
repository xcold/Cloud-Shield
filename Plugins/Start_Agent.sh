#! /bin/bash

if [ `whoami` == "root" ]; then 
	echo "增加Daemon执行权限" 
	chmod 777 * 
	echo "运行守护进程" 
	./Daemon>output_startagent 2>&1 & 
else
	echo "不是root权限,请使用root权限启动程序" 
fi

