#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
用于采集服务器mac
'''
import os,json
MAC=os.popen("""ip a|grep link/ether|awk '{print $2}'""").read()
d={"mac":MAC.strip("\n").split("\n")}
print d
