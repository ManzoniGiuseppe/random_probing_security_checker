#SHARES 3
#IN a
#RANDOMS r0 r1 r2 r3
#OUT c

x0 = a0 + r0
x1 = a1 + r1
s0 = r0 + r1
x2 = a2 + s0

y0 = 1 + r2
s1 = r2 + r3

c0 = y0 + x0
c1 = r3 + x1
c2 = s1 + x2
