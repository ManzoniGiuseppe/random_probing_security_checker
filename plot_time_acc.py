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
import os
import matplotlib
from matplotlib import pyplot as plt


# handle the gadgets

def evalFn(p, coeff):
  ps = [1]
  nps = [1]
  for i in range(1, len(coeff)):
    ps += [ps[len(ps)-1] * p]
    nps += [nps[len(nps)-1] * (1-p)]

  ret = 0.0
  for i in range(len(coeff)):
    ret += ps[i] * nps[len(coeff)-1 -i] * coeff[i]
  return ret

def findTreshold(coeff):
  p = 0.001
  while evalFn(p, coeff) < p and p < 1.0:
    p += 0.001
  p -= 0.001
  return p

def getMaxCoeffStatus(dir):
  if os.path.exists(dir + str(chosenMaxCoeff) + '.success'):
    return (0, chosenMaxCoeff)
  for c in range(chosenMaxCoeff+1):
     if os.path.exists(dir + str(c) + '.timeout'):
       return (-1, c-1)
     if os.path.exists(dir + str(c) + '.otherfail'):
       return (-2, c-1)
  return (-3, c-1) # uncompleted

def getInfo(factor, maxCoeff, d, undIns, dir):
  (status, chosenCoeff) = getMaxCoeffStatus(dir)

  if status != 0:
    return (status, -1, -1)

  file = dir + str(chosenCoeff) + '.success'
  maskedIns = d * undIns

  coeffs_high = []
  coeffs_low = []

  maxPresent = int(file.split('/')[-1].split('.')[0])

  with open(file) as f:
    file_content = f.read().splitlines()
    in_coeffs = file_content[0].split()
    for i in range(len(in_coeffs)):
      coeffs_high += [float(in_coeffs[i])]
      if(i <= maxPresent):
        coeffs_low += [float(in_coeffs[i])]
      else:
        coeffs_low += [(math.comb(maxCoeff - maskedIns, i - maskedIns) * factor if i >= maskedIns else 0.0)]
    time = file_content[1]

  p_low = findTreshold(coeffs_high) # high coeff -> low p
  p_high = findTreshold(coeffs_low)

  min = float(time.split(':')[0])
  sec = float(time.split(':')[1][:-1])  # [:-1] to remove final s
  return (min * 60 + sec, p_low, p_high)



# get info to plot


prefix = sys.argv[1]
what = sys.argv[2:]

gadgetInfo = []
for it in what:
  gadgetInfo += [it.split(':')]


print(gadgetInfo)

toPrint = []
if prefix == 'rps':
  toPrint += [[[], [], [], [], 'VRAPS', 'o', '.vraps.out', 'Vraps']]
  toPrint += [[[], [], [], [], 'COR3', '+', '.rpsc.out', 'Cor3']]
  toPrint += [[[], [], [], [], 'COR2', '*', '.rpsc.out', 'Cor2']]
  toPrint += [[[], [], [], [], 'COR1', 'x', '.rpsc.out', 'Cor1']]
if prefix == 'rpc':
  toPrint += [[[], [], [], [], 'COR1', 'o', '.rpsc.out', 'Cor1']]
  toPrint += [[[], [], [], [], 'COR2', '+', '.rpsc.out', 'Cor2']]
  toPrint += [[[], [], [], [], 'VRAPS', 'x', '.vraps.out', 'Vraps']]


toPrint_p = 0
toPrint_p_err = 1
toPrint_time = 2
toPrint_color = 3
toPrint_name = 4
toPrint_marker = 5
toPrint_dir = 6
toPrint_op = 7


timeouts = [0] * len(toPrint)
otherfails = [0] * len(toPrint)
uncompleted = [0] * len(toPrint)


for (num, (gadget, chosenMaxCoeff, color)) in enumerate(gadgetInfo):
  chosenMaxCoeff = int(chosenMaxCoeff)
  print('processing', gadget)
  sagefile = 'gadgets/' + gadget

  if '__' in sagefile:
    sagefile = '<(' + sagefile.replace('__', ' ') + ')'

  maxCoeff = int(subprocess.check_output(['bash', '-c', './rpsc --sage ' + sagefile + ' --printGadget | grep "^NUM_TOT_PROBES=" | sed "s/^NUM_TOT_PROBES=//"']))
  d = int(subprocess.check_output(['bash', '-c', './rpsc --sage ' + sagefile + ' --printGadget | grep "^D=" | sed "s/^D=//"']))
  undIns = int(subprocess.check_output(['bash', '-c', './rpsc --sage ' + sagefile + ' --printGadget | grep "^NUM_INS=" | sed "s/^NUM_INS=//"']))

  if prefix == 'rpc':
    suffix = '__' + str((d-1)//2)
  else:
    suffix = ''

  for i in range(len(toPrint)):
    if prefix == 'rpc':
      t = (d + 1)/2
      factor = 1 - 2**(-undIns * (d - t)) if toPrint[i][toPrint_name] != 'RPC_VRAPS' else 1.0
    else:
      factor = 1 - 2**(-undIns) if toPrint[i][toPrint_name] != 'RPS_VRAPS' else 1.0

    (time, p_low, p_high) = getInfo(0.0, maxCoeff, d, undIns, toPrint[i][toPrint_dir] + '/' + gadget + '/' + prefix + toPrint[i][toPrint_op] + suffix + '/')
    if time == -1:
      timeouts[i] += 1
      print('  ' + toPrint[i][toPrint_name] + ': timeout')
    elif time == -2:
      otherfails[i] += 1
      print('  ' + toPrint[i][toPrint_name] + ': otherfail')
    elif time == -3:
      uncompleted[i] += 1
      print('  ' + toPrint[i][toPrint_name] + ': uncompleted')
    else:
      toPrint[i][toPrint_p] += [p_low]
      toPrint[i][toPrint_p_err] += [p_high - p_low]
      toPrint[i][toPrint_time] += [time]
      toPrint[i][toPrint_color] += [num]
      print('  ' + toPrint[i][toPrint_name] + ': ' + str(p_low) + '~' +  str(p_high) + ' - ' + str(time))

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

fig, ax = plt.subplots(figsize=(10, 6))

ax.set_title(prefix)

ax.set_xlabel('time (s)')
ax.set_ylabel('max p : f(p)<=p')
ax.set_xscale('log')
ax.set_yscale('log')
ax.grid(which='both')

cmap, norm = matplotlib.colors.from_levels_and_colors(np.arange(0, len(gadgetInfo)+1), [c for g,k,c in gadgetInfo])

scatter = []
for it in toPrint:
  scatter += [ ax.scatter(x=it[toPrint_time], y=it[toPrint_p], c=it[toPrint_color], label=it[toPrint_name], marker=it[toPrint_marker], cmap=cmap, norm=norm) ]


# add legend


legend1 = ax.legend(title="Implementation", loc='center right')
#legend1 = ax.legend(title="Implementation", loc='upper right')
ax.add_artist(legend1)

legend_elements = scatter[1].legend_elements()

#legend2 = ax.legend(legend_elements[0], [gadgetInfo[int(i[14:-2])][0].replace('__', ' d=') + ' c=' + str(gadgetInfo[int(i[14:-2])][1]) for i in legend_elements[1]], title="Gadget", loc='upper left')
legend2 = ax.legend(legend_elements[0], [gadgetInfo[int(i[14:-2])][0].replace('__', ' d=') + ' c=' + str(gadgetInfo[int(i[14:-2])][1]) for i in legend_elements[1]], title="Gadget", loc='lower right')
ax.add_artist(legend2)


# add error

for it in toPrint:
  for j in range(len(it[toPrint_p_err])):
#    ax.errorbar(x=[it[toPrint_time][j]], y=[it[toPrint_p][j]], yerr=[[0.0], [it[toPrint_p_err][j]] ], xerr = 0, marker=it[toPrint_marker], fmt='o--', c=gadgetInfo[ it[toPrint_color][j] ][2], linewidth=0.7)
    a=1

# save

#plt.ylim(0.01, 0.1)
plt.savefig('.plot_time_acc.out.png', bbox_inches='tight')#, pad_inches=0)
