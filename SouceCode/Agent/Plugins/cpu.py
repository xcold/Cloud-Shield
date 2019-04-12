#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集服务器CPU信息，core为CPU核数，CPUmodel为CPU型号
'''
import os,json
CPUNUM=os.popen("""cat /proc/cpuinfo |grep processor|wc -l""").read()
CPUSORT=os.popen("cat /proc/cpuinfo |grep \"model name\"|awk -F':' '{print $2}'|uniq").read()
d={"core":CPUNUM.strip("\n"),"CPUmodel":CPUSORT.strip("\n")}
print d
