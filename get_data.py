import os
from time import sleep
import subprocess
import re

NUM_BURSTS = 1

def getValues(data):
    out = str(data).split("\n")
    lat_value = 0
    avg_value = 0
    for i in out:
        if re.match("percentile_values_ns.*", i.strip()):
            lat_value = int(i.split(':')[-1])
        if re.match("avg_ns.*", i.strip()):
            avg_value = int(i.split(':')[-1])
    return [lat_value, avg_value]

def stat():
    for pps in range(410000, 480000, 5000):
        os.system("./bin/bessctl 'run samples/stat BESS_PKT_RATE="+str(pps)+",STAT_MODE=1'")
        sleep(5)
        os.system("./bin/bessctl 'command module stat reset_cycle_count EmptyArg'")
        sleep(5)
        for i in range(NUM_BURSTS):
            os.system("./bin/bessctl 'command module src set_burst_size SourceCommandSetBurstSizeArg {\"burst_size\":1}'")
            sleep(5)
        os.system("./bin/bessctl command module stat get_cycles EmptyArg > output")
#        os.system("./bin/bessctl 'command module ms get_summary MeasureCommandGetSummaryArg {\"latency_percentiles\":[99]}' > output")
        (out,err) = subprocess.Popen(["cat", "output"], stdout=subprocess.PIPE).communicate()
        '''
        lat_value, avg_value = getValues(out)
        with open("data",'a') as f:
            f.write("%d,%d,%d\n"%(pps,lat_value,avg_value))
        '''
        with open("data",'a') as f:
            f.write("%d\n"%(int(out.split(':')[-1])))


def anamoly():
    for pps in range(1000000, 1050000, 1000):
        os.system("./bin/bessctl 'run samples/flow_drr BESS_PKT_RATE="+str(pps)+"'")
        sleep(5)
        os.system("./bin/bessctl 'command module ms clear EmptyArg'")
#        sleep(5)
#        os.system("./bin/bessctl 'command module src set_burst_size SourceCommandSetBurstSizeArg {\"burst_size\":1}'")
        sleep(20)
        os.system("./bin/bessctl 'command module ms get_summary MeasureCommandGetSummaryArg {\"latency_percentiles\":[99]}' > output")
        (out,err) = subprocess.Popen(["cat", "output"], stdout=subprocess.PIPE).communicate()
        lat_value, avg_value = getValues(out)
        with open("cpu_util_max",'a') as f:
            f.write("%d,%d,%d\n"%(pps,lat_value,avg_value))

anamoly()


