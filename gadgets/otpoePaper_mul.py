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

print(*['r'+str(i)+str(j) for i in range(numShares) for j in range(numShares)], *['q'+str(i)+str(j) for i in range(numShares) for j in range(numShares)], sep=' ')
print('#OUT c')


for i in range(numShares):
  for j in range(numShares):
    next = j+1 if j+1 < numShares else 0
    print('t0 = q',i,j,' + q',i,next, sep='')
    print('d',i,j,' = b',j,' + t0', sep='')

for i in range(numShares):
  for j in range(numShares):
    print('p',i,j,' = a',i,' * d',i,j, sep='')
    print('p',i,j,' = p',i,j,' + r',i,j, sep='')

for i in range(numShares):
  print('v',i,' = p',i,'0 + p',i,'1', sep='')
  print('x',i,' = r',i,'0 + r',i,'1', sep='')
  for j in range(2,numShares):
    print('v',i,' = v',i,' + p',i,j, sep='')
    print('x',i,' = x',i,' + r',i,j, sep='')

for i in range(numShares):
  print('c',i,' = v',i,' + x',i, sep='')
