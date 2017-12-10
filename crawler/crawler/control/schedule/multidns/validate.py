import os
import sys
import socket
import codecs
import html5lib
import traceback
import re
from bs4 import BeautifulSoup
from task_runner import map_task
from chardet import universaldetector

dns = []
sites = []

for line in open('top_sites.txt', 'rb'):
  line = line.strip()
  if line:
    sites.append(line)

def source():
  return sites

def detect_encoding(string):
  detector = universaldetector.UniversalDetector()
  detector.reset()
  detector.feed(string)
  detector.close()
  if detector.result and detector.result['confidence'] >= 0.6:
    return detector.result['encoding']
  return 'gb18030'

def title_by_content_old(content):
  encoding = detect_encoding(content)
  s = codecs.decode(content, encoding, 'replace')
  soup = BeautifulSoup(s, "html5lib")
  if soup.title:
    return soup.title.string

def title_by_content(content):
  if content is None:
    return None
  content = content.lower().replace('\n', ' ')
  content = content.replace('\r', ' ').replace('\t', ' ')
  r = re.search('<title>(.*?)</title>',content)
  if r:
    gps = r.groups()
    if len(gps) > 0 and gps[0] is not None:
      title = gps[0].replace(' ', '')
      if title:
        return title
  return None


def get_title(hostname, ip, url):
  try:
    path = url[url.find('/', 7):]
    remote = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    remote.connect((ip, 80))
    remote.settimeout(6)
    remote = remote.makefile()
    remote.write('GET %s HTTP/1.1\n' % path)
    remote.write('Host: %s\n' % hostname)
    remote.write('User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:12.0) Gecko/20100101 Firefox/13.0\n')
    remote.write('Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\n')
    remote.write('Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3i\n')
    remote.write('Connection: close\n\n')
    remote.flush()
    s = remote.read()
    remote.close()
    title = title_by_content(s)
    return title
  except Exception as e:
    print e
    return None

def expand_ip(ips, a_site):
  if a_site != 'zhidao.baidu.com':
    return ips
  results = []
  for ip in ips:
    results.append(ip)
    nums = ip.split('.')
    if len(nums) == 4:
      host = int(nums[3])
      if host > 1:
        results.append('%s.%s.%s.%d' % tuple(nums[0:3] + [host - 1]))
      if host < 254:
        results.append('%s.%s.%s.%d' % tuple(nums[0:3] + [host + 1]))
  return set(results)

def run(a_site):
  try:
    a_site, url = a_site.split('\t')
    with open('merge/' + a_site, 'rb') as f:
      print a_site
      title = get_title(a_site, a_site, url)
      if title is None:
        # try again
        title = get_title(a_site, a_site, url) or get_title(a_site, a_site, url)
      print title
      ips = []
      for line in f:
        ip = line.strip()
        ips.append(ip)
      count = 0
      for ip in expand_ip(ips, a_site):
        if count > 20:
          break
        print ip, a_site
        a_title = get_title(a_site, ip, url) or get_title(a_site, ip, url)
        print a_title, title
        if title is not None and title == a_title:
          count += 1
          good_f = open('validate/' + a_site, 'ab')
          good_f.write(ip + '\t' + a_title.replace('\n', ' ') + '\t' + title.replace('\n', ' ') + '\n')
          good_f.close()
        else:
          bad_f = open('error/' + a_site, 'ab')
          bad_f.write(ip + '\t' + str(a_title).replace('\n', ' ') + '\t' + str(title).replace('\n', ' ') + '\n')
          bad_f.close()
  except Exception as e:
    print e


if __name__ == "__main__":
    map_task(source(), run, 40)

