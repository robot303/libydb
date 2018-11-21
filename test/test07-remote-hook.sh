#!/bin/sh
. ./util.sh
test_init $0 $1
echo -n "TEST: $TESTNAME : "
run_bg "ydb -r pub -d -N -f ../examples/yaml/ydb-sample.yaml -v info > $TESTNAME.PUB1.log"
run_bg "ydb-hook-example remote-hook > $TESTNAME.PUB2.log"
sleep 1
r1=`ydb -r sub -N --unsubscribe --sync-before-read --read /xe1/enabled`
r2=`ydb -r sub -N --unsubscribe --sync-before-read --read /xe1/enabled`

test_deinit

if [ "value_$r1" =  "value_true" ];then
    if [ "value_$r2" =  "value_false" ];then
        echo "ok ($r1, $r2)"
        exitcode=0
    else
        echo "failed ($r1, $r2)"
        exitcode=1
    fi
    
else
    echo "failed ($r1, $r2)"
    exitcode=1
fi
exit $exitcode