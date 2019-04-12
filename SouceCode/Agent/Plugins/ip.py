#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集服务器IP
'''
import os,json
IP=os.popen("""ip a|grep "inet"|grep -v inet6|grep -v 127.0.0.1|awk '{print $2}'|awk -F "/" '{print $1}'""").read()
d={"ip":IP.strip("\n").split("\n")}
print d
