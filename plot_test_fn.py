#!/usr/bin/env python3


import numpy as np
import subprocess
import math
import sys
from matplotlib import pyplot as plt


x = 10 ** np.arange(-5.00, 0.0, 0.1)

for file in sys.stdin:
  file = file[:-1]

  sagefile = 'gadgets/' + file.split('/')[1]
  print(sagefile)

  if '=' in sagefile:
    sagefile = '<(' + sagefile.replace('=', ' ') + ')'

  maxCoeff = int(subprocess.check_output(['bash', '-c', './exec.sh -s ' + sagefile + ' -c 0 --no-compile | grep "^GCC FLAGS: " | sed "s/-D/\\n/g" | grep "^TOT_MUL_PROBES=" | sed "s/^TOT_MUL_PROBES=//"']))
  print(maxCoeff)

  coeffs = [math.comb(maxCoeff, i) * 1.0 for i in range(maxCoeff+1)]

  with open(file) as f:
    in_coeffs = f.read().splitlines()[0].split()
    for i in range(len(in_coeffs)):
      coeffs[i] = float(in_coeffs[i])

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
plt.legend()
plt.show()

