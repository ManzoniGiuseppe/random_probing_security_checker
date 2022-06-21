#SHARES 3
#IN a b
#RANDOMS r0 r1 r2 r3
#OUT c

x0 = a0 + r0
x1 = a1 + r1
s0 = r0 + r1
x2 = a2 + s0

y0 = b0 + r2
y1 = b1 + r3
s1 = r2 + r3
y2 = b2 + s1

c0 = y0 + x0
c1 = y1 + x1
c2 = y2 + x2
