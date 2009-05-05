#!/bin/bash
PROCESSID=`ps | grep xpce | awk '{print $1}'`

if [ $PROCESSID ]
then
    echo Kill process $PROCESSID
    kill $PROCESSID
else
    echo xpce process is not running
fi

