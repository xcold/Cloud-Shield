#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
用于监控服务器是否被登录
'''
import os,json,sys
def cur_file_dir():
     path = sys.path[0]
     if os.path.isdir(path):
         return path
     elif os.path.isfile(path):
         return os.path.dirname(path)
User=os.popen("""whoami""").read().strip("\n")
IP=os.popen("""ip a|grep "inet"|grep -v inet6|grep -v 127.0.0.1|awk '{print $2}'|awk -F "/" '{print $1}'""").read().strip("\n")
Date=os.popen('date +"%F %T"').read().strip("\n")
iftty=os.popen("""last|grep "logged in"|grep `whoami`|grep tty|wc -l""").read().strip("\n")
if iftty == str(0):
		ip=os.popen("""last -a|grep "logged in"|grep `whoami`|grep -v tty|awk -F " " '{print $NF}'|sort -u""").read().strip("\n")
else:
		ip="there is someone logined through the console"
#d={"TITLE":ip+" "+Date" "+User login+IP}
d={"TITLE":User+" has logined in "+IP+" at "+Date+" from "+ip}
print (json.dumps([d],encoding="UTF-8",ensure_ascii=False))

