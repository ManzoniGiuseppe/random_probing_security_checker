#SHARES 2
#IN a b
#RANDOMS r0
#OUT d

c0 = a0 * b0	
d0 = c0 + r0

c1 = a1 * b1
c1 = c1 + r0
tmp = a0 * b1
c1 = c1 + tmp
tmp = a1 * b0

d1 = c1 + tmp
