#!/bin/bash

cd crawler/control/robots/

url_field=0
robots_level_field=2

donot_allow=0
has_no_robots=1
others_allow=2
allow=3

# 特殊 case, 测试是否可抓取, 新加的看看能否被抓取都在这里测试
# I'am GeneralSpider, output can fetch
spider_type=GeneralSpider
cat test/test1 | python robots_parser_mapper.py ${url_field} ${spider_type} -100 1 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://translate.google.com.vn/#en|德语|hour	google 翻译
http://car.auto.ifeng.com/photo/carpic/7378/1527254
http://car.auto.ifeng.com/" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "360spider fail" >&2
  exit 1
fi

# 固定case, 测试各个环节
# I'am 360Spider, output can fetch
spider_type=360Spider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} ${robots_level_field} 1 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://www.xx.com/	g	${allow}
http://www.xx.com/file/	h	${allow}
http://www.zz.com/	l	${has_no_robots}
http://www.zz.com/file	m	${has_no_robots}" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "360spider fail" >&2
  exit 1
fi

# I'am 360Spider, output can fetch
spider_type=360Spider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} 1 1 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://www.xx.com/	${allow}	g
http://www.xx.com/file/	${allow}	h
http://www.zz.com/	${has_no_robots}	l
http://www.zz.com/file	${has_no_robots}	m" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "360spider fail" >&2
  exit 1
fi

# I'am 360Spider, output cannot fetch
spider_type=360Spider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} ${robots_level_field} 0 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://zhidao.baidu.com/	a	${others_allow}
http://zhidao.baidu.com	b	${others_allow}
http://zhidao.baidu.com/question/473777148.html	c	${others_allow}
http://www.baidu.com	d	${others_allow}
http://www.baidu.com/	e	${others_allow}
http://www.baidu.com/baidu?wd=q	f	${donot_allow}
http://www.yy.com	i	${donot_allow}
http://www.yy.com/	j	${donot_allow}
http://www.yy.com/file/	k	${donot_allow}" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "360spider, output cannot fetch fail" >&2
  exit 1
fi

# I'am GeneralSpider, output can fetch
spider_type=GeneralSpider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} ${robots_level_field} 1 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://zhidao.baidu.com/	a	${others_allow}
http://zhidao.baidu.com	b	${others_allow}
http://zhidao.baidu.com/question/473777148.html	c	${others_allow}
http://www.baidu.com	d	${others_allow}
http://www.baidu.com/	e	${others_allow}
http://www.xx.com/	g	${allow}
http://www.xx.com/file/	h	${allow}
http://www.zz.com/	l	${has_no_robots}
http://www.zz.com/file	m	${has_no_robots}" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "GeneralSpider fail" >&2
  exit 1
fi

# I'am GeneralSpider, output cannot fetch
spider_type=GeneralSpider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} ${robots_level_field} 0 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://www.baidu.com/baidu?wd=q	f	${donot_allow}
http://www.yy.com	i	${donot_allow}
http://www.yy.com/	j	${donot_allow}
http://www.yy.com/file/	k	${donot_allow}" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "GeneralSpider, output cannot fetch fail" >&2
  exit 1
fi

# I'am Rushing, output can fetch
spider_type=RushSpider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} ${robots_level_field} 1 > 1
[[ $? -ne 0 ]] && exit 1
echo "http://zhidao.baidu.com/	a	${others_allow}
http://zhidao.baidu.com	b	${others_allow}
http://zhidao.baidu.com/question/473777148.html	c	${others_allow}
http://www.baidu.com	d	${others_allow}
http://www.baidu.com/	e	${others_allow}
http://www.baidu.com/baidu?wd=q	f	${donot_allow}
http://www.xx.com/	g	${allow}
http://www.xx.com/file/	h	${allow}
http://www.yy.com	i	${donot_allow}
http://www.yy.com/	j	${donot_allow}
http://www.yy.com/file/	k	${donot_allow}
http://www.zz.com/	l	${has_no_robots}
http://www.zz.com/file	m	${has_no_robots}" > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "RushSpider fail" >&2
  exit 1
fi

# I'am Rushing, output cannot fetch
spider_type=RushSpider
cat test/samples.test | python robots_parser_mapper.py ${url_field} ${spider_type} ${robots_level_field} 0 > 1
[[ $? -ne 0 ]] && exit 1
cat /dev/null > 2
diff 1 2
if [ $? -ne 0 ];then
  echo "RushSpider output cannot fetch fail" >&2
  exit 1
fi
rm 1 2 *.pyc
exit 0
