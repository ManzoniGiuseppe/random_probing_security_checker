#!/usr/bin/env python3

import sys

if len(sys.argv) != 2:
  print("missing num shares")
  sys.exit(1)

numShares = int(sys.argv[1])

if numShares <= 1:
  print('at least 2 shares')
  sys.exit(1)

print('#SHARES', numShares)
print('#IN a b')
print('#RANDOMS ', end='')

print(*['z'+str(i)+str(j) for j in range(numShares) for i in range(j)], sep=' ')
print('#OUT c')

for j in range(numShares):
  for i in range(numShares):
    print('w',i,j,' = a',i,' * b',j, sep='')

for j in range(numShares):
  for i in range(j):
    print('z',j,i,' = z',i,j,' + w',i,j, sep='')
    print('z',j,i,' = z',j,i,' + w',j,i, sep='')

def get_coeff(i,j):
  if i != j:
    return 'z'+str(i)+str(j)
  else:
    return 'w'+str(i)+str(j)

for i in range(numShares):
  if numShares == 2:
    print('c',i,' = ', get_coeff(i,0), ' + ', get_coeff(i,1), sep='')
  else:
    print('d',i,' = ', get_coeff(i,0), ' + ', get_coeff(i,1), sep='')
    for j in range(2,numShares-1):
      print('d',i,' = d',i,' + ', get_coeff(i,j), sep='')
    print('c',i,' = d',i,' + ', get_coeff(i,numShares-1), sep='')

