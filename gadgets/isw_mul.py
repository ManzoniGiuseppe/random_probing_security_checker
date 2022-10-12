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
print('#IN a b')
print('#RANDOMS ', end='')

print(*['r'+str(i)+str(j) for i in range(numShares) for j in range(i+1,numShares)], sep=' ')
print('#OUT c')


for i in range(numShares):
  print('c',i,' = a',i,' * b',i, sep='')

for i in range(numShares):
  for j in range(i+1,numShares):
    print('c',i,' = c',i,' + r',i,j, sep='')

    print('w',i,j,' = a',i,' * b',j, sep='')
    print('r',j,i,' = w',i,j,' + r',i,j, sep='')

    print('w',j,i,' = a',j,' * b',i, sep='')
    print('r',j,i,' = r',j,i,' + w',j,i, sep='')

    print('c',j,' = c',j,' + r',j,i, sep='')
