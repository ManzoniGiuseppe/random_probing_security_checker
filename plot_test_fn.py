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




x = 10 ** np.arange(-5, -0.0, 0.01)

plt.loglog(x,x,label='id')

for file in sys.argv[1:]:
  sagefile = 'gadgets/' + file.split('/')[1]
  print(sagefile)

  if '__' in sagefile:
    sagefile = '<(' + sagefile.replace('__', ' ') + ')'

  coeffs = []

  with open(file) as f:
    in_coeffs = f.read().splitlines()[0].split()
    for i in range(len(in_coeffs)):
      coeffs += [ float(in_coeffs[i]) ]
  maxCoeff = len(coeffs)-1

  xs = [x * 0 + 1]
  nxs = [x * 0 + 1]
  for i in range(1,maxCoeff+1):
    xs += [xs[len(xs)-1] * x]
    nxs += [nxs[len(nxs)-1] * (1-x)]

  y = 0
  for i in range(maxCoeff+1):
    y += xs[i] * nxs[maxCoeff-i] * coeffs[i]

  plt.loglog(x,y,label=file)

plt.xlabel('p')
plt.ylabel('f(p)')
plt.legend(loc='lower right')
#plt.ylim(0.0000001, 1.05)
#plt.xlim(0.001, 0.0)
plt.show()

