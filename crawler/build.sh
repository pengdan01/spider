#!/bin/sh

cd `dirname $0`
WORKING_DIR=`pwd`

CC_PUB_DIR=/data10/crawler-cpp/cc-pub

# 首先安装libaio库，编译依赖
sudo yum install -y libaio-devel

# 安装blade build文件，去掉jvm依赖
cp -f build_tools/blade3/Makefile.header $CC_PUB_DIR/build_tools/blade3/Makefile.header

# 将爬虫的fetcher2库和price_recg库copy过去
cd $CC_PUB_DIR
find . -name fetcher2 | xargs -I{} rm -rf {}
find . -name price_recg | xargs -I{} rm -rf {}
cd $WORKING_DIR
cp -rf pub /data10/crawler-cpp/cc-pub/

# 将pub库都list出来
cd $CC_PUB_DIR
sh ./list_pub_libs.sh

# 将爬虫的东西link到cc-pub里
cd $CC_PUB_DIR
rm -vf crawler crawler2 log_analysis
ln -s $WORKING_DIR/crawler crawler
ln -s $WORKING_DIR/crawler2 crawler2
ln -s $WORKING_DIR/log_analysis log_analysis
find pub -name "libcurl.a" | xargs -I{} cp ${WORKING_DIR}/curl/lib/libcurl.a {}
rm -rf pub/src/third_party/curl
mkdir pub/src/third_party/curl
cp -r ${WORKING_DIR}/curl/include pub/src/third_party/curl/
cp -r ${WORKING_DIR}/curl/BUILD pub/src/third_party/curl/

./gen_makefile.sh spider/crawler/BUILD 
make

if [ $? -eq 0 ]; then
  mkdir -p $WORKING_DIR/.build
  CMD="cp $CC_PUB_DIR/.build/dbg/targets/spider/crawler/crawler $WORKING_DIR/.build/"
  echo $CMD
  $CMD
fi
