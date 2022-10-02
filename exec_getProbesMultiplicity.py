#!/usr/bin/env python3

import sys
import re


if len(sys.argv) != 3:
  print("args: <probes offset> <num probes>")
  sys.exit(1)

offset = int(sys.argv[1])
numProbes = int(sys.argv[2])

mRet = re.compile(r'ret\[([0-9]*)\]')
mOp = re.compile(r'[+*]')
mult = numProbes*[0]
for line in sys.stdin:
  toAdd = 2 if mOp.search(line) else 1

  for indexStr in mRet.findall(line)[1:]:  # ignore the return, only put probes on the inputs
    index = int(indexStr)
    if index >= offset:
      mult[index-offset] += toAdd

mult=[(it-1 if it > 0 else 0) for it in mult]

print(mult)
