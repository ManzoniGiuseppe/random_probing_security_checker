#!/usr/bin/env python3


import numpy as np
import subprocess
import math
import sys
import os
from matplotlib import pyplot as plt


prefix = sys.argv[1]
suffix = sys.argv[2]
chosenMaxCoeff = int(sys.argv[3])

def evalFn(p, coeff):
  ret = 0.0
  p_v = 1.0
  p_n = 1.0
  for i in range(len(coeff)):
    ret += p_v * p_n * coeff[i]
    p_v *= p
    p_n *= 1 - p
  return ret

def findTreshold(coeff):
  up = 1.0
  down = 0.0
  while True:
    mid = (up + down) / 2

    if up - down < 2**-20:
      return mid

    f_mid = evalFn(mid, coeff)
    if f_mid == mid:
      return mid
    elif f_mid > mid:
      up = mid
    else:
      down = mid

def getMaxCoeffStatus(dir):
  if os.path.exists(dir + str(chosenMaxCoeff) + '.success'):
    return (0, chosenMaxCoeff)
  for c in range(chosenMaxCoeff+1):
    if os.path.exists(dir + str(c) + '.timeout'):
      return (-1, c-1)
    if os.path.exists(dir + str(c) + '.otherfail'):
      return (-2, c-1)
  return (-3, c-1) # uncompleted

def getInfo(maxCoeff, dir):
  coeffs = [math.comb(maxCoeff, i) * 1.0 for i in range(maxCoeff+1)]

  (status, chosenCoeff) = getMaxCoeffStatus(dir)

  if status != 0:
    return (status, status)

  file = dir + str(chosenCoeff) + '.success'

  with open(file) as f:
    file_content = f.read().splitlines()
    in_coeffs = file_content[0].split()
    for i in range(len(in_coeffs)):
      coeffs[i] = float(in_coeffs[i])
    time = file_content[1]

  p = findTreshold(coeffs)

  min = float(time.split(':')[0])
  sec = float(time.split(':')[1][:-1])  # [:-1] to remove final s
  return (p, min * 60 + sec)




toPrint = []
toPrint += [([], [], 'M0', 'o', 'test', 'Is')]
if sys.argv[1] == 'rps':
  toPrint += [([], [], 'MGM', '^', 'test', 'Sum')]
toPrint += [([], [], 'Mteo', 'x', 'test', 'Teo')]
toPrint += [([], [], 'vraps', '.', 'vraps', 'Vraps')]

timeouts = [0] * len(toPrint)
otherfails = [0] * len(toPrint)
uncompleted = [0] * len(toPrint)

for gadget in next(os.walk('test'))[1]:
  print('processing', gadget)
  sagefile = 'gadgets/' + gadget

  if '=' in sagefile:
    sagefile = '<(' + sagefile.replace('=', ' ') + ')'

  maxCoeff = int(subprocess.check_output(['bash', '-c', './exec.sh -s ' + sagefile + ' -c 0 --no-compile | grep "^GCC FLAGS: " | sed "s/-D/\\n/g" | grep "^TOT_MUL_PROBES=" | sed "s/^TOT_MUL_PROBES=//"']))

  for i in range(len(toPrint)):
    (p, time) = getInfo(maxCoeff, toPrint[i][4] + '/' + gadget + '/' + prefix + toPrint[i][5] + suffix + '/')
    if p == -1:
      timeouts[i] += 1
      print('  ' + toPrint[i][2] + ': timeout')
    elif p == -2:
      otherfails[i] += 1
      print('  ' + toPrint[i][2] + ': otherfail')
    elif p == -3:
      uncompleted[i] += 1
      print('  ' + toPrint[i][2] + ': uncompleted')
    else:
      toPrint[i] = (toPrint[i][0] + [p], toPrint[i][1] + [time]) + toPrint[i][2:6]
      print('  ' + toPrint[i][2] + ': ' + str(p) + ' - ' + str(time))

for it in toPrint:
  plt.scatter(it[1], it[0], label=it[2], marker=it[3])

print('timeouts:')
for i in range(len(toPrint)):
  print('  ' + toPrint[i][2] + ': ' + str(timeouts[i]))
print('otherfails:')
for i in range(len(toPrint)):
  print('  ' + toPrint[i][2] + ': ' + str(otherfails[i]))
print('uncompleted:')
for i in range(len(toPrint)):
  print('  ' + toPrint[i][2] + ': ' + str(uncompleted[i]))

plt.xlabel('time (s)')
plt.ylabel('max p : f(p)<=p')
plt.xscale('log')
plt.yscale('log')
plt.legend()
plt.grid(which='both')
plt.show()

