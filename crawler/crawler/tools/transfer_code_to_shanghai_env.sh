#!/bin/bash

set -u

target="180.153.227.26:~/tmp/"

bash -x crawler/gen_crawler_product_env.sh

if [ $? -ne 0 ]; then
  echo "bash -x crawler/gen_crawler_product_env.sh fail"
  exit 1
fi

scp -r wly_crawler_online.tar.gz "${target}"

if [ $? -ne 0 ]; then
  echo "scp fail"
  exit 1
fi

echo "Done!"
exit 0
