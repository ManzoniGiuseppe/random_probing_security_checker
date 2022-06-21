#SHARES 3
#IN a b
#RANDOMS r0 r1 r2 r3 r4 r5
#OUT d

c0 = r3 + r4
c0 = c0 + r1
c0 = c0 + r0
c0 = c0 + b0
d0 = c0 + a0

c1 = r4 + r5
c1 = c1 + r2
c1 = c1 + r1
c1 = c1 + a1
d1 = c1 + b1

c2 = r5 + r3
c2 = c2 + r0
c2 = c2 + r2
c2 = c2 + a2
d2 = c2 + b2

