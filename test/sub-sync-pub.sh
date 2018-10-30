#!/bin/sh
TESTNAME=`basename "$0"`
echo -n "TEST: $TESTNAME : "
ydb -r pub -d -s -N -f ../examples/yaml/ydb-sample.yaml > ydb.pub.log &
publisher=$!
sleep 1
r1=`ydb -r sub -N --unsubscribe --read /2/2-1/2-1-1`
r2=`ydb -r sub -N --unsubscribe --read /1/1-2/1-2-3`
kill -2 $publisher
# echo $r1
if [ "value_$r1" =  "value_v7" ];then
    if [ "value_$r2" =  "value_v6" ];then
        echo "ok ($r1, $r2)"
        exit 0
    else
        echo "failed ($r1, $r2)"
        exit 1
    fi
    
else
    echo "failed ($r1, $r2)"
    exit 1
fi
