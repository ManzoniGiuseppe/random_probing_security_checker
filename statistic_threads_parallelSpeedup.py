#!/usr/bin/env python3
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.


import numpy as np
import subprocess
import math
import sys
from matplotlib import pyplot as plt


allArr = {}

with open('.rpsc.out.stat') as f:
  for line in f.read().splitlines():
    v = line.split(':')

    file=v[0]
    threads = int(file.split('/')[0].split('-')[1])
    what = "/".join(file.split('/')[1:])

    if what not in allArr:
      allArr[what] = []

    speedupParallelized = float(v[1])
    parallelized = float(v[2])

    allArr[what] += [(threads, speedupParallelized)]

for w in allArr:
  arr = allArr[w]

  x = [i[0] for i in arr]
  y = [i[1] for i in arr]

  plt.plot(x,y)


plt.xlabel('threads')
plt.ylabel('sp: the speedup of the parallelized section')
#plt.legend(loc='upper left')
#plt.ylim(0.0000001, 1.05)
#plt.xlim(0.001, 0.0)
plt.show()

