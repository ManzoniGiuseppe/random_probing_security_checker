#!/usr/bin/env python3


import numpy as np
import subprocess
import math
import sys
import os
import matplotlib
from matplotlib import pyplot as plt


# handle the gadgets

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





# get info to plot



prefix = sys.argv[1]
suffix = sys.argv[2]
chosenMaxCoeff = int(sys.argv[3])
gadgetInfo = [('vrapsPaper_copy.sage', 'b'),
              ('otpoePaper_copy.sage', 'tab:blue'),
              ('otpoePaper_add.sage', 'limegreen'),
              ('vrapsPaper_add_v1.sage', 'g'),
              ('vrapsPaper_add_v2.sage', 'lime'),
              ('isw_mul_generator.py=4', 'red'),
              ('isw_mul_generator.py=3', 'darkred'),
              ('vrapsPaper_mul.sage', 'tab:orange'),
              ('otpoePaper_mul.sage', 'tomato'),
              ('otpoePaper_refresh.sage', 'grey'),
              ('refresh_ring.sage', 'black')]


toPrint = []
if prefix == 'rps':
  toPrint += [[[], [], [], 'RPS_VRAPS', 'o', 'vraps', 'Vraps']]
  toPrint += [[[], [], [], 'RPS_COR3', '+', 'test', 'Is']]
  toPrint += [[[], [], [], 'RPS_COR2', '*', 'test', 'Sum']]
  toPrint += [[[], [], [], 'RPS_COR1', 'x', 'test', 'Teo']]
if prefix == 'rpc':
  toPrint += [[[], [], [], 'SD_COR', 'o', 'test', 'Teo']]
  toPrint += [[[], [], [], 'VRAPS_COR', '+', 'test', 'Is']]
  toPrint += [[[], [], [], 'VRAPS_ORIG', 'x', 'vraps', 'Vraps']]


toPrint_p = 0
toPrint_time = 1
toPrint_color = 2
toPrint_name = 3
toPrint_marker = 4
toPrint_dir = 5
toPrint_op = 6


timeouts = [0] * len(toPrint)
otherfails = [0] * len(toPrint)
uncompleted = [0] * len(toPrint)

for (color, gadget) in enumerate([g for g,c in gadgetInfo]):
  print('processing', gadget)
  sagefile = 'gadgets/' + gadget

  if '=' in sagefile:
    sagefile = '<(' + sagefile.replace('=', ' ') + ')'

  maxCoeff = int(subprocess.check_output(['bash', '-c', './exec.sh -s ' + sagefile + ' -c 0 --no-compile | grep "^GCC FLAGS: " | sed "s/-D/\\n/g" | grep "^TOT_MUL_PROBES=" | sed "s/^TOT_MUL_PROBES=//"']))

  for i in range(len(toPrint)):
    (p, time) = getInfo(maxCoeff, toPrint[i][toPrint_dir] + '/' + gadget + '/' + prefix + toPrint[i][toPrint_op] + suffix + '/')
    if p == -1:
      timeouts[i] += 1
      print('  ' + toPrint[i][toPrint_name] + ': timeout')
    elif p == -2:
      otherfails[i] += 1
      print('  ' + toPrint[i][toPrint_name] + ': otherfail')
    elif p == -3:
      uncompleted[i] += 1
      print('  ' + toPrint[i][toPrint_name] + ': uncompleted')
    else:
      toPrint[i][toPrint_p] += [p]
      toPrint[i][toPrint_time] += [time]
      toPrint[i][toPrint_color] += [color]
      print('  ' + toPrint[i][toPrint_name] + ': ' + str(p) + ' - ' + str(time))

print('timeouts:')
for i in range(len(toPrint)):
  print('  ' + toPrint[i][toPrint_name] + ': ' + str(timeouts[i]))
print('otherfails:')
for i in range(len(toPrint)):
  print('  ' + toPrint[i][toPrint_name] + ': ' + str(otherfails[i]))
print('uncompleted:')
for i in range(len(toPrint)):
  print('  ' + toPrint[i][toPrint_name] + ': ' + str(uncompleted[i]))



# scatterplot

fig, ax = plt.subplots(figsize=(20, 15))

ax.set_xlabel('time (s)')
ax.set_ylabel('max p : f(p)<=p')
ax.set_xscale('log')
ax.set_yscale('log')
ax.grid(which='both')

cmap, norm = matplotlib.colors.from_levels_and_colors(np.arange(0, len(gadgetInfo)+1), [c for g,c in gadgetInfo])

scatter = []
for it in toPrint:
  scatter += [ ax.scatter(x=it[toPrint_time], y=it[toPrint_p], c=it[toPrint_color], label=it[toPrint_name], marker=it[toPrint_marker], cmap=cmap, norm=norm) ]



# add legend

box = ax.get_position()
ax.set_position([box.x0, box.y0, box.width * 0.9, box.height])


legend1 = ax.legend(title="Implementation", loc='upper left', bbox_to_anchor=(1, 0.5))
ax.add_artist(legend1)

legend_elements = scatter[1].legend_elements()

legend2 = ax.legend(legend_elements[0], [gadgetInfo[int(i[14:-2])][0] for i in legend_elements[1]], title="Gadget", loc='lower left', bbox_to_anchor=(1, 0.5))
ax.add_artist(legend2)

# save

plt.savefig('toDelete_out.pdf', bbox_inches='tight')#, pad_inches=0)

