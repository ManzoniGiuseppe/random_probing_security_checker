#!/usr/bin/env python3

import sys
import re
import math

if len(sys.argv) != 4:
  print("params: <num underlying ins> <d> <t>")
  sys.exit(1)


numIns = int(sys.argv[1])
d = int(sys.argv[2])
t = int(sys.argv[3])

inComb = sum(math.comb(d, it) for it in range(t+1))**numIns

print(inComb)
