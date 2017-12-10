#!/usr/bin/python
# -*- encoding: utf-8 -*-

import os
import sys
import random
import string
import array
import profile

def isprint(c):
  return c >= 32 and c <= 126
def isxdigit(c):
  return (47 < c < 58) or (64 < c < 91) or (96 < c < 123)
def isoctaldigit(c):
  return 47 < c < 56
def hex_digit_to_int(c):
  if c > 57: return (c + 9) & 15
  else: return c & 15
def digit_to_hex(c):
  if c > 9: return chr(c + 87)
  else: return chr(c + 48)

def escape(src, use_hex = 1, utf8_safe = 1):
  dest = array.array('c')
  for char in src:
    acc = ord(char)
    if char == '\n':
      dest.append('\\')
      dest.append('n')
    elif char == '\r':
      dest.append('\\')
      dest.append('r')
    elif char == '\t':
      dest.append('\\')
      dest.append('t')
    elif char == '\\':
      dest.append('\\')
      dest.append('\\')
    elif (utf8_safe == 0 or (acc >= 0 and acc < 128)) and (isprint(acc) == False):
      dest.append('\\')
      if use_hex:
        dest.append('x')
        dest.append(digit_to_hex(acc>>4))
        dest.append(digit_to_hex(acc&15))
      else:
        dest.append(chr(acc>>6))
        dest.append(chr(acc>>3)&7)
        dest.append(chr(acc&7))
    else: dest.append(char)
  return dest.tostring()

def unescape(src):
  dest = array.array('c')
  flag = 0
  tox = 0
  too = 0
  for char in src:
    if flag == 0:
      if char != '\\': dest.append(char)
      else: flag = 1
    elif flag == 1:
      flag = 0
      if char == 'a': dest.append('\a')
      elif char == 'b': dest.append('\b')
      elif char == 'f': dest.append('\f')
      elif char == 'n': dest.append('\n')
      elif char == 'r': dest.append('\r')
      elif char == 't': dest.append('\t')
      elif char == 'r': dest.append('\r')
      elif char == '?': dest.append('\?')
      elif char == '\'': dest.append('\'')
      elif char == '\"': dest.append('\"')
      elif char == '\\': dest.append('\\')
      elif char == 'x' or char == 'X': flag = 2
      else:
        acc = ord(char)
        if isoctaldigit(acc):
          too = acc&15
          flag = 4
        else: flag = -1
    elif flag == 2:
      acc = ord(char)
      if isxdigit(acc):
        tox = hex_digit_to_int(acc)
        flag = 3
      else: flag = -1
    elif flag == 3:
      acc = ord(char)
      if isxdigit(acc):
        tox = (tox << 4)|hex_digit_to_int(acc)
        dest.append(chr(tox))
        flag = 0
      else: flag = -1
    elif flag == 4:
      acc = ord(char)
      if isoctaldigit(acc):
        flag = 5
        too = (too<<3)|(acc&15)
      else: flag = -1
    elif flag == 5:
      acc = ord(char)
      if isoctaldigit(acc):
        flag = 0
        too = (too<<3)|(acc&15)
        dest.append(chr(too))
      else: flag = -1
    else:
     raise ValueError("bad format") 
  return dest.tostring()

def rand_samples():
  original = array.array('c')
  for i in range(256):
    original.append(chr(i))
  for i in xrange(0, 1000000):
    original.append(chr((i + random.randint(0, 256))%256))
  return original

def main():
  original = rand_samples()
  run_test(original)

def run_test(original):
  en = escape(original)
  ou = unescape(en)
  if original != ou:
    print "error"
  else:
    print "done"
if __name__ == "__main__" :
  main()
##original = rand_samples()
##def perf_test():
##  run_test(original)

##profile.run('perf_test()')

