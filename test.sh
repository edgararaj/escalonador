#!/bin/bash

rm -f tmp/* /tmp/escalonador*
time bin/orchestrator tmp 3 FCFS && ./mean_time.sh tmp/completed.txt &

bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 100 -u 'test/hello 6' > /dev/null
bin/client execute 10 -u 'test/hello 2' > /dev/null
bin/client execute 0 -u '' > /dev/null # terminate the orchestrator when all tasks terminate