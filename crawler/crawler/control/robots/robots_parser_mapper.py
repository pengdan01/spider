#!/usr/bin/python
# -*- encoding: utf-8 -*-

import os
import sys
import pdb
import robotexclusionrulesparser
import line_escape
import urlparse
import urllib

def LoadRobots (content, rp):
  dest = line_escape.unescape(content)
#  rp.parse(dest)
  arr = dest.split('\n')
  for i, item in enumerate(arr):
    if item.strip() == 'Allow:':
      arr[i] = "Allow: /"
    if arr[i].find('**') != -1:
      b = arr[i].split('*')
      l = len(b)
      arr[i] = '*'.join([elem for j,elem in enumerate(b) if j == 0 or j == l-1 or elem != ""])

  rp.parse('\n'.join(arr))

def CanFetch (url, rp):
  flds = url.split("/")
  relative = ''
  if len(flds) == 3:
    relative = '/'
  else:
    relative = '/' + '/'.join(flds[3:])

  someotherspider_allowed = False
  oneboxspider_allowed = False
  can_fetch = True 
  #pdb.set_trace()
  if True == rp.is_allowed('360Spider', relative):
    can_fetch = True
    oneboxspider_allowed = True
    someotherspider_allowed = True
  else:
    can_fetch = False
  if can_fetch == False:
    if True == rp.is_allowed('Baiduspider', relative) or \
       True == rp.is_allowed('Googlebot', relative):
      can_fetch = True
      someotherspider_allowed = True
  return (can_fetch, oneboxspider_allowed, someotherspider_allowed)

def ReadSequential (url_field, robots_level_field, spider_type, output_can_fetch):
  rp = robotexclusionrulesparser.RobotExclusionRulesParser()

  robots_host = ''
  robots_loaded = False
  robots_raw_content = ''

  every_n_idx = 0
  every_n = 100000

  for line in sys.stdin:
  # for line in open('test/samples.test'):
    #pdb.set_trace()
    array = line.strip('\n').split('\t')
    cur_host = array[0]
    tag = array[1]
    url = ''
    if tag is 'A':
      robots_host = cur_host
      robots_loaded = False
      robots_raw_content = array[2]
      continue
    else:
      url = urllib.quote(array[url_field], safe='/@#?:')

    #pdb.set_trace()
    can_fetch = True
    oneboxspider_allowed = False
    someotherspider_allowed = False
    has_robots_txt = False
    if cur_host == robots_host:
      if robots_loaded == False:
        LoadRobots(robots_raw_content, rp)
        robots_loaded = True
      has_robots_txt = True
      (can_fetch, oneboxspider_allowed, someotherspider_allowed) = CanFetch(url, rp)

    robots_level = 0
    if can_fetch == False:
      robots_level = 0
    elif has_robots_txt == False:
      robots_level = 1
    elif oneboxspider_allowed == False and someotherspider_allowed == True:
      robots_level = 2
    elif oneboxspider_allowed == True:
      robots_level = 3
    else:
      sys.stderr.write("invalid robots_level")
      sys.exit(1)

    if spider_type == 3:
      can_fetch = True
    elif spider_type ==1 and can_fetch == True and has_robots_txt == True and oneboxspider_allowed == False:
      can_fetch = False

    if can_fetch == output_can_fetch:
      if robots_level_field > 0:
        array.insert(robots_level_field, str(robots_level))
      output_line = "\t".join(array[2:])
      sys.stdout.write('%s\n' % (output_line))
    else:
# 打印到标准错误的日志
      if robots_level_field > 0:
        array.insert(robots_level_field, str(robots_level))
      output_line = "\t".join(array[2:])
      if every_n_idx % every_n == 0:
        sys.stderr.write('%s\n' % (output_line))
        every_n_idx = 0
      every_n_idx += 1

if __name__ == '__main__':
  if len(sys.argv) < 5:
    sys.stderr.write("4 cmd paras expected\n")
    sys.exit(1)

  url_field = int(sys.argv[1])
  url_field += 2

# general_spider: I am transformer of any spider, eg. Baiduspider, GoogleBot..
# 360 Spider, I am 360Spider
# RushSpider, I am rushing, what is robots.txt?
  spider = sys.argv[2]
  if spider != 'GeneralSpider' and \
     spider != '360Spider' and \
     spider != 'RushSpider':
    sys.stderr.write('Invalid input, spider')
    sys.exit(1)

  spider_type = 1
  if spider == '360Spider':
    spider_type = 1
  elif spider == 'GeneralSpider':
    spider_type = 2
  else:
    spider_type = 3

  robots_level_field = int(sys.argv[3])
  robots_level_field += 2
# 1, 输出 can fetch
# 其他, 输出 cannot fetch
  output_type = int(sys.argv[4])
  if output_type == 1:
    output_can_fetch = True
  else:
    output_can_fetch = False

  ReadSequential(url_field, robots_level_field, spider_type, output_can_fetch)


