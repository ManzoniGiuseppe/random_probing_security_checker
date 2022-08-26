#!/usr/bin/env python3

import sys
import re

if len(sys.argv) != 2:
  print("missing num shares")
  sys.exit(1)

numVar = int(sys.argv[1]) # first probe of the variables
numOut = 0 # the real outputs are always the firsts
mSep = re.compile(r' *[=+*] *')
probe = {}

for line in sys.stdin:
  line = line.rstrip('\n\r')
  v = mSep.split(line)
  if '*' in line:
    op=' * '
  else:
    op=' + '

  if v[1] in probe:
    v[1] = probe[v[1]]
  if v[2] in probe:
    v[2] = probe[v[2]]

  if '!' in v[0][0:1]:
    v[0] = v[0][1:]
    probe[v[0]] = 'ret[' + str(numOut) + ']'
    numOut += 1
  else:
    probe[v[0]] = 'ret[' + str(numVar) + ']'
    numVar += 1

  print(probe[v[0]] + ' = ' + v[1] + op + v[2])
