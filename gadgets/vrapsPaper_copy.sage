#ORDER 1
#SHARES 3
#IN a
#RANDOMS r0 r1 r2 r3 r4 r5
#OUT c e

b0 = a0 + r0
b1 = a1 + r1
b2 = a2 + r2

c0 = b0 + r1
c1 = b1 + r2
c2 = b2 + r0

d0 = a0 + r3
d1 = a1 + r4
d2 = a2 + r5

e0 = d0 + r4
e1 = d1 + r5
e2 = d2 + r3
