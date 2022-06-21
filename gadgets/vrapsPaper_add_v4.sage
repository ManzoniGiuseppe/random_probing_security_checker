#SHARES 3
#IN a b
#RANDOMS r0 r1 r2 r3 r4 r5
#OUT c

x0 = a0 + r0
x0 = x0 + r4
x1 = a1 + r1
x1 = x1 + r5
x2 = a2 + r2
x2 = x2 + r3

y0 = b0 + r1
y0 = y0 + r3
y1 = b1 + r2
y1 = y1 + r4
y2 = b2 + r0
y2 = y2 + r5

c0 = x0 + y0
c1 = x1 + y1
c2 = x2 + y2
