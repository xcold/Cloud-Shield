#!/usr/bin/env python
# encoding: utf-8

import os
import time
try:
    import psutil
except ImportError:
    psutil = None


class CPUCollector():

    PROC = '/proc/stat'
    INTERVAL = 1

    def __init__(self):
        self.ret = {}
    def collect(self):
        """
        get cpu time list
        """

        def cpu_time_list():
            statFile = open(self.PROC, 'r')
            timeList = statFile.readline().split(" ")[2:6]
            for i in range(len(timeList)):
                timeList[i] = int(timeList[i])
            statFile.close()
            return timeList

        def cpu_delta_time(interval):

            pre_check = cpu_time_list()
            time.sleep(interval)
            post_check = cpu_time_list()
            for i in range(len(pre_check)):
                post_check[i] -= pre_check[i]
            return post_check

        if os.access(self.PROC, os.R_OK):
            dt = cpu_delta_time(self.INTERVAL)
            cpuPct = 100 - (dt[len(dt) - 1] * 100.00 / sum(dt))
            self.ret['percent'] = '{:.4f}'.format(cpuPct)

            file = open(self.PROC)
            ncpus = -1
            for line in file:
                if not line.startswith('cpu'):
                    continue

                ncpus += 1
                elements = line.split()
                cpu = elements[0]

                if cpu == 'cpu':
                    cpu = 'cpu'
                if len(elements) >= 2:
                    self.ret[cpu+'.user'] = elements[1]
                if len(elements) >= 3:
                    self.ret[cpu+'.nice'] = elements[2]
                if len(elements) >= 4:
                    self.ret[cpu+'.system'] = elements[3]
                if len(elements) >= 5:
                    self.ret[cpu+'.idle'] = elements[4]
                if len(elements) >= 6:
                    self.ret[cpu+'.iowait'] = elements[5]
                if len(elements) >= 7:
                    self.ret[cpu+'.irq'] = elements[6]
                if len(elements) >= 8:
                    self.ret[cpu+'.softirq'] = elements[7]
                if len(elements) >= 9:
                    self.ret[cpu+'.steal'] = elements[8]
                if len(elements) >= 10:
                    self.ret[cpu+'.guest'] = elements[9]
                if len(elements) >= 11:
                    self.ret[cpu+'.guest_nice'] = elements[10]
            file.close()
            return self.ret

        else:
            return None

def main():
    s = CPUCollector()
    ret = s.collect()
    result = []
    for k,v in ret.items():
            result.append((k, v))
    return result

if __name__ == '__main__':
    print main()
