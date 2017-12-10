#!/bin/bash
make clean
[ $? -ne 0 ] && exit 1
./gen_makefile.sh crawler/updater/BUILD
[ $? -ne 0 ] && exit 1
make opt -j8
[ $? -ne 0 ] && exit 1
rm -rf ./updater_online
mkdir -p ./updater_online
cp -r build_tools/mapreduce ./updater_online/
cp -r .build ./updater_online/
cp crawler/updater/updater.conf ./updater_online/
cp crawler/updater/updater.sh ./updater_online/
rm -rf updater_online.tar.gz
tar -czvf updater_online.tar.gz ./updater_online/* ./updater_online/.build
scp updater_online.tar.gz 180.153.227.26:~/tmp
[ $? -ne 0 ] && exit 1
exit 0
