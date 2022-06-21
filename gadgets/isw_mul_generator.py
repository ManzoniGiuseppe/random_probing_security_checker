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

if numShares == 2:
  print('c0 = w00 + z01')
  print('c1 = w11 + z10')
else:
  for i in range(numShares):
    list = [str(j) for j in range(numShares) if j != i]
    print('d',i,' = z',i,list[0], ' + z',i,list[1], sep='')
    for it in list[2:]:
      print('d',i,' = d',i,' + z',i,it, sep='')
    print('c',i,' = w',i,i,' + d',i, sep='')

