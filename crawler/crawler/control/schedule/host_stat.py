import os
import sys

url_dict={}

while line in sys.stdin:
  line_arr = line.split("\t")
  url_dict[line_arr[1]] += 1
