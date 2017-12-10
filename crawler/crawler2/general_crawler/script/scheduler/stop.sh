#!/bin/bash

source shell/conf.sh

kill -9 `cat $lockfile`
kill -9 `cat $pidfile`
rm $lockfile
rm $pidfile