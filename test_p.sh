#!/bin/bash

# check if argument is either FCFS or SJF
if [ "$1" != "FCFS" ] && [ "$1" != "SJF" ]; then
    echo "Usage: $0 <FCFS|SJF> <n>"
    exit 1
fi

rm -f tmp/* /tmp/escalonador*
time bin/orchestrator tmp 3 $1 > /dev/null && ./mean_time.sh tmp/completed.txt && echo '' &

# create a for loop to execute this commands n times with decreasing values
for i in $(seq 1 $2);
do
    bin/client execute $((i * 100)) -u 'test/hello '$i > /dev/null
done

bin/client execute 0 -u '' > /dev/null # terminate the orchestrator when all tasks terminate

# wait for the orchestrator to finish
wait