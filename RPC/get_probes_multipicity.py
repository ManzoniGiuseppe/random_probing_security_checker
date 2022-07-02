#!/usr/bin/env python3

import sys
import re


if len(sys.argv) != 3:
  print("args: <probes offset> <num probes>")
  sys.exit(1)

offset = int(sys.argv[1])
numProbes = int(sys.argv[2])

mLine = re.compile(r'ret\[([0-9]*)\] = ret\[([0-9]*)\] [+*] ret\[*([0-9]*)\]')
mult = numProbes*[0]
for line in sys.stdin:
  v = mLine.search(line).groups()
  i = [int(v[1]), int(v[2])]
  for j in i:
    if j >= offset:
      mult[j-offset] += 1

mult=[it*2-1 for it in mult]

print(mult)

