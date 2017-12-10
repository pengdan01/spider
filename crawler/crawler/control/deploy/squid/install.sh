#!/bin/bash

set -u

prefix=/home/crawler/squid/squid
cache=/home/crawler/squid/cache

cd `dirname $0` &&
tar zxf squid-3.1.20.tar.gz &&
cd squid-3.1.20 &&
./configure --with-maxfd=65536 --with-pthreads --enable-epoll --disable-poll --prefix=$prefix &&
make -j8 &&
make install &&
cd .. &&
cp -f squid.conf $prefix/etc &&
mkdir -p $cache &&
$prefix/sbin/squid -zX &&
$prefix/sbin/squid

