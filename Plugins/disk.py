#!/usr/bin/python
#coding:utf-8
#version 1.0
'''
采集硬盘信息，disk为硬盘总大小，dsize为硬盘的组成，RAID为阵列
'''
import os,json
ifmcli=os.popen("""dpkg -l | grep megacli | wc -l""").read().strip("\n")
if ifmcli == str(1):
    DISKTYPE=os.popen("""megacli -PDList -aALL|grep "Raw Size"|uniq -c|awk -F " " '{print $4 " " $5 "*" $1}'""").read()
    Raid=os.popen("""megacli -LDInfo -Lall -aALL|grep \"RAID Level\"|awk -F \": \" '{print $2}'""").read().strip("\n")
else:
    DISKTYPE=os.popen("""fdisk -l|grep GB|awk -F " " '{print $3}'|uniq -c|awk -F " " '{print $2"GB""*" $1}'""").read()
DISKTOA=os.popen("""fdisk -l|grep Disk|head -1|awk -F ": |," '{print $2}'""").read()
iFLG=os.popen("""dmidecode -s system-serial-number | grep -E "VMware|Specified" | wc -l""").read().strip("\n")
if iFLG == str(1):
    raid=""
else:
    if Raid == "Primary-1, Secondary-0, RAID Level Qualifier-0" :
        raid="RAID1"
    elif Raid == "Primary-0, Secondary-0, RAID Level Qualifier-0" :
        raid="RAID0"
    elif    Raid == "Primary-5, Secondary-0, RAID Level Qualifier-3" :
        raid="RAID5"
    elif Raid == "Primary-1, Secondary-3, RAID Level Qualifier-0" :
        raid="RAID10"
d={"disk":DISKTYPE.strip("\n").split("\n"),"dsize":DISKTOA.strip("\n"),"RAID":raid}
print d
