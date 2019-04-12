#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集服务端口信息
'''
import os,json
PROT=os.popen("""netstat -tulnp | grep -v Active | grep -v Proto | grep -v '.*:::\*.*' | awk -F'[*/]|:+| +' '{print $1,$5,$(NF-1)}' | sed 's/^[tu][cd][p].*LISTEN.*//g' | grep -v ^$  | uniq|grep -v '[0-9]\{1,3\}$'""").read()
d={"port":PROT.strip("\n").split("\n")}
print d
