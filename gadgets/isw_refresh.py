#!/usr/bin/env python3

import sys

if len(sys.argv) != 2:
  print("missing num shares")
  sys.exit(1)

numShares = int(sys.argv[1])

if numShares <= 2:
  print('at least 3 shares')
  sys.exit(1)

print('#SHARES', numShares)
print('#IN a')
print('#RANDOMS ', end='')

print(*['r'+str(i)+str(j) for i in range(numShares) for j in range(i+1,numShares)], sep=' ')
print('#OUT c')


print('c0 = a0 + r01')
for j in range(2,numShares):
  print('c0 = c0 + r0',j, sep='')

for i in range(1,numShares):
  print('c',i,' = a',i,' + r0',i, sep='')
  for j in range(1,i):
    print('c',i,' = c',i,' + r',j,i, sep='')
  for j in range(i+1,numShares):
    print('c',i,' = c',i,' + r',i,j, sep='')
