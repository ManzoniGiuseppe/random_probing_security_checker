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

print(*['r0'+str(i) for i in range(numShares)], *['r1'+str(i) for i in range(numShares)], sep=' ')
print('#OUT c')

for i in range(numShares):
  next = i+1 if i+1 < numShares else 0
  print('t0 = r0',i,' + r0',next, sep='')
  print('d',i,' = a',i,' + t0', sep='')

for i in range(numShares):
  next = i+1 if i+1 < numShares else 0
  print('t0 = r1',i,' + r1',next, sep='')
  print('e',i,' = b',i,' + t0', sep='')

for i in range(numShares):
  print('c',i,' = d',i,' + e',i, sep='')

