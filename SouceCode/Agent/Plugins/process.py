#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集进程信息
'''
import os,json
PROCESS=os.popen("""ps auxww|awk -F " " '{print $1 " " $11 " " $12" "$13" " $14" " $15" "$16}'|grep -v USER |grep -v ps|grep -v awk""").read()
d={"processes":PROCESS.strip("\n").split("\n")}
print d
