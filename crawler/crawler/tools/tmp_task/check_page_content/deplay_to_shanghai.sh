#!/bin/bash
ssh "crawler@m04" "cd ~/pengdan/ && [ ! -d check_res_comment_price ] && mkdir -p check_res_comment_price"

rsync -cavz check_valid_page.sh crawler@m04:~/pengdan/check_res_comment_price/
[ $? -ne 0 ] && exit 1
rsync -cavz ~/workplace/wly/.build/opt/targets/crawler/tools/tmp_task/mr_check_valid_page_mapper crawler@m04:~/pengdan/check_res_comment_price/
[ $? -ne 0 ] && exit 1

rsync -cavz ~/workplace/wly/crawler2/price_recg/data crawler@m04:~/pengdan/check_res_comment_price/
[ $? -ne 0 ] && exit 1
exit 0
