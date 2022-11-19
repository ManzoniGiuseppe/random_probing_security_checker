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


specialNames = ['add: {vrapsV3,otpoe3} rps{2,3}', 'mul: {isw3 rpsCor1,vraps rpcCor1}', 'add,copy,refresh', 'mul']

arrSpecialID = {
  'add: {vrapsV3,otpoe3} rps{2,3}': ['vrapsPaper_add_v3.sage/rpsCor2', 'vrapsPaper_add_v3.sage/rpsCor3', 'otpoePaper_add.py__3/rpsCor2', 'otpoePaper_add.py__3/rpsCor3' ],
  'mul: {isw3 rpsCor1,vraps rpcCor1}': ['isw_mul.py__3/rpsCor1','vrapsPaper_mul.sage/rpcCor1__1'],
  'add,copy,refresh': ['add', 'copy', 'refresh'],
  'mul': ['mul'],
}

gadget2arr = {
  'add: {vrapsV3,otpoe3} rps{2,3}': [],
  'mul: {isw3 rpsCor1,vraps rpcCor1}': [],
  'add,copy,refresh': [],
  'mul': [],
}

gadget2color = {
  'add: {vrapsV3,otpoe3} rps{2,3}': 'black',
  'mul: {isw3 rpsCor1,vraps rpcCor1}': 'blue',
  'add,copy,refresh': 'orange',
  'mul': 'grey',
}

with open('.rpsc.out.stat') as f:
  for line in f.read().splitlines():
    v = line.split(':')

    file=v[0]
    fn = (v[0].split('/')[2] + '_').split('_')[0]
    gadget = v[0].split('/')[1]

    parallelSpeedup = float(v[1])
    parallelized = float(v[2])

    found=False
    for special in specialNames:
      for type in arrSpecialID[special]:
        if type in file:
          if not found:
            gadget2arr[special] += [(parallelized, parallelSpeedup)]
            found=True
    if not found:
      print("Not added:", file)


for gadget in gadget2arr:
  arr = gadget2arr[gadget]
  print(gadget, arr)

  x = [i[0] for i in arr]
  y = [i[1] for i in arr]

  plt.scatter(x,y,label=gadget,color=gadget2color[gadget])


plt.xlabel('p: fraction of time being parallelized')
plt.ylabel('sp: speedup of the parallelized fraction')
plt.legend(loc='upper left')
#plt.ylim(0.0000001, 1.05)
#plt.xlim(0.001, 0.0)
plt.show()

