#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集服务器硬件信息，snum为设备编号，size为外形，smodel为型号，manufacturer为厂商
'''
import os,json
iFLG=os.popen("""dmidecode -s system-serial-number | grep -E "VMware|Specified" | wc -l""").read().strip("\n")
if iFLG == str(1):
    FAC=""
    SM=""
    SMSORT=""
    U_NUM=""
else:
    FAC=os.popen("""dmidecode -s system-manufacturer|grep -v ^#|awk '{print $1}'""").read()
    SM=os.popen("""dmidecode -s system-serial-number|grep -v ^#""").read()
    SMSORT=os.popen("""dmidecode -s system-product-name|grep -v ^#""").read()
    U_NUM=os.popen("""dmidecode -t chassis |grep Height:|cut -d : -f 2""").read()
d={"manufacturer":FAC.strip("\n"),"snum":SM.strip("\n"),"smodel":SMSORT.strip("\n"),"size":U_NUM.strip("\n")}
print d
