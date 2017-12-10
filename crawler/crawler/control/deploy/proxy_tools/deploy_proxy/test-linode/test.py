import os
import sys
import datetime
import time
from task_runner import map_task

def timestamp():
  return time.time()


def source():
  f = open('url.txt', 'rb')
  for line in f:
    yield line.split('\t')[0]

s = 0

total = 0
success = 0
failure = 0
last_success = 0
last_timestamp = timestamp()

def run(i):
  global s, total, success, failure, last_timestamp, last_success
  r = os.system(
    'curl "%s" --socks5 localhost:1080 -y 300 --connect-timeout 300 --retry 0 >output/"%s" 2>/dev/null </dev/null' % (
      i, '%d' % s))
  total += 1
  if r == 0:
    success += 1
  else:
    failure += 1
  s += 1
  t = timestamp()
  if total % 100 == 0:
    print >> sys.stderr, 'total: %d\tsuccess: %d\tfailures: %d\tQPS: %f' % (
      total, success, failure, (success - last_success) / (t - last_timestamp + 0.000000001))
    last_timestamp = t
    last_success = success

if __name__ == "__main__":
  map_task(source(), run, 500)
