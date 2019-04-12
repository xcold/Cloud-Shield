#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集服务器登录失败IP信息
'''
import os,json
FSIP=os.popen("""lastb -i -a | grep ssh  | awk '{print $NF}' | uniq  -c | awk '$1>3{print $2,$1}'|sort -k2 -n -r""").read()
d={"lastb":FSIP.strip("\n").split("\n")}
print d
