#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集系统信息，sysver为系统版本，kernel为内核信息
'''
import os,json
Dist=os.popen("""lsb_release -a 2>/dev/null|grep "Distributor ID"|awk -F " " '{print $3}'""").read()
ReLEse= os.popen("""lsb_release -a 2>/dev/null|grep "Release"|awk -F " " '{print $2}'""").read()
BIT=os.popen("""getconf LONG_BIT""").read()
CODE=os.popen("""uname -r""").read()
d={"sysver":Dist.strip("\n")+ " "+ReLEse.strip("\n")+" "+BIT.strip("\n")+"bit","kernel":CODE.strip("\n")}
print d
