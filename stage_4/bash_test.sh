#!/bin/bash
if [ "$#" != "2" ]; then
    echo "Usage: sh bash_test.sh <nb_clients> <nb_msgs>"
fi
if [ "$#" == "2" ]; then
    ./follow_test.run -t $2 >> /tmp/f1.log 2>&1
    grep -B2 -i --color 'warn\|fail\|err' /tmp/f1.log
    rm /tmp/f1.log
    ./follow_test.run -t $2 -s >> /tmp/f2.log 2>&1
    grep -B2 -i --color 'warn\|fail\|err' /tmp/f2.log
    rm /tmp/f2.log
    ./stress_test.run -n $1 -k $2 >> /tmp/s1.log 2>&1
    grep -B2 -i --color 'warn\|fail\|err' /tmp/s1.log
    rm /tmp/s1.log
    ./stress_test.run -n $1 -k $2 -s >> /tmp/s2.log 2>&1
    grep -B2 -i --color 'warn\|fail\|err' /tmp/s2.log
    rm /tmp/s2.log
fi

