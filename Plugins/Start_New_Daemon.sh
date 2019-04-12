#! /bin/bash

echo "start Start_New_Daemon.sh"

# 增加可执行权限
chmod 777 /usr/Agent/*
chmod 777 /usr/Agent/Plugins/*

cd /usr/Agent/

/usr/Agent/Daemon>output_new_daemon 2>&1 &

echo "end Start_New_Daemon.sh"
