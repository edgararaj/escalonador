#!/bin/bash

# check if argument is either FCFS or SJF
if [ "$1" != "FCFS" ] && [ "$1" != "SJF" ]; then
    echo "Usage: $0 <FCFS|SJF>"
    exit 1
fi

rm -f tmp/* /tmp/escalonador*
time bin/orchestrator tmp 3 $1 && ./mean_time.sh tmp/completed.txt && echo '' &

bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 10 -u 'test/hello 2' > /dev/null
bin/client execute 0 -u '' > /dev/null # terminate the orchestrator when all tasks terminate