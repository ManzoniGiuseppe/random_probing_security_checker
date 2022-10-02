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
print('#OUT c')

for i in range(numShares):
  print('c',i,' = a',i, sep='')
