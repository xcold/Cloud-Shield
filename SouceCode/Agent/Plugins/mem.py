#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集内存信息，mem为内存大小
'''
import os,json,string
IFMEM=os.popen("""free -g|grep Mem:|awk -F " " '{print $2 }'""").read().strip("\n")
MEMORY=os.popen("""free -g|grep Mem:|awk -F " " '{print $2 "G"}'""").read()
if IFMEM == str(0): 
        MEMORY=os.popen("""free -m|grep Mem:|awk -F " " '{print $2 "M"}'""").read()
d={"mem":MEMORY.strip("\n") }
print d
