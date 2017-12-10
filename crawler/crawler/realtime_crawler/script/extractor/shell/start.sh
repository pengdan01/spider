#!/bin/bash

source shell/conf.sh
source shell/common.sh

set -u

pid=""

mkdir -p run

if [ -f $lockfile ]; then
    echo "Has been a instance running, pid: `tail -n1 $lockfile`"
    exit 1
else
    touch $lockfile
    echo "$$" > $lockfile
fi

if [ ! -f $pidfile ]; then
    touch $pidfile;
fi

OnExit() {
    kill -9 $pid
    rm $lockfile
}

trap OnExit Exit

while true
do
    $binary $params &
    echo $!
    echo $! > ${pidfile};
    pid=$!
    wait $!

    sleep ${time_to_restart};

    err_msg="[realtime_extractor]: $binary abnormal quit and restart"
    SendAlertViaMail "${err_msg}" "${alert_receiver}"
done
