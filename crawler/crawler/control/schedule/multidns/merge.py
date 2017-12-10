import os
import sys
from task_runner import map_task

sites = []

for line in open('top_sites.txt', 'rb'):
  line = line.strip()
  if line:
    sites.append(line.split('\t')[0])

for a_site in sites:
  f = open('output/' + a_site, 'rb')
  f2 = open('tmp/' + a_site, 'wb')
  start = False
  for line in f:
    line = line.strip()
    if ';; ANSWER SECTION:' == line:
      start = True
      continue
    if not line:
      start = False
      continue
    if start and line.find('IN\tA') > 0:
      print >>f2, line.split('\t')[-1]
  f2.close()
  f.close()
  os.system('sort < tmp/"%s" | uniq > merge/"%s"' % (a_site, a_site))

