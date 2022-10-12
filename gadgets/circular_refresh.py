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
print('#IN a')
print('#RANDOMS ', end='')

print(*['r'+str(i) for i in range(numShares)], sep=' ')
print('#OUT c')

for i in range(numShares):
  next = i+1 if i+1 < numShares else 0
  print('t0 = r',i,' + r',next, sep='')
  print('c',i,' = a',i,' + t0', sep='')
