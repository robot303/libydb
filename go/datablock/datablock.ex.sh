#!/bin/bash

# ydb -r pub -a uss://test -d -f ../../github.com/neoul/libydb/examples/yaml/yaml-sequence.yaml &
ydb -r pub -a uss://test -d -f datablock.ex.yaml &
PUBPID=$!

# cd ydb.ex
go run datablock.ex.go
kill $PUBPID
cd -