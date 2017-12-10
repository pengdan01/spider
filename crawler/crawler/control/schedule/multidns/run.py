import os
import sys
from task_runner import map_task

dns = []
sites = []

for line in open('dns.txt', 'rb'):
  line = line.strip()
  if line:
    dns.append(line)

for line in open('top_sites.txt', 'rb'):
  line = line.strip()
  if line:
    sites.append(line.split('\t')[0])

def source():
  for a_dns in dns:
      yield a_dns

def run(a_dns):
  fail_count = 0
  for a_site in sites:
    if os.system('dig @"%s" +time=001 "%s" >> output/"%s"' % (a_dns, a_site, a_site)) != 0:
      #os.system('echo "%s" >> output/"%s"' % (a_dns, a_site))
      fail_count += 1
    else:
      fail_count -= 1
    if fail_count > 10:
      break

if __name__ == "__main__":
    map_task(source(), run, 100)

