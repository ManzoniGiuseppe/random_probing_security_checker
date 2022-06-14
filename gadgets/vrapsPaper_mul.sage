#SHARES 3
#IN a b
#RANDOMS r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10
#OUT c

u0 = a0 + r5
u0 = u0 + r6
u1 = a1 + r6
u1 = u1 + r7
u2 = a2 + r7
u2 = u2 + r5

v0 = b0 + r8
v0 = v0 + r9
v1 = b1 + r9
v1 = v1 + r10
v2 = b2 + r10
v2 = v2 + r8

w00 = u0 * v0
w01 = u0 * v1
w02 = u0 * v2
w10 = u1 * v0
w11 = u1 * v1
w12 = u1 * v2
w20 = u2 * v0
w21 = u2 * v1
w22 = u2 * v2

w00 = w00 + r0
w01 = w01 + r1
w02 = w02 + r2
w10 = w10 + r1
w11 = w11 + r4
w12 = w12 + r3
w20 = w20 + r2
w21 = w21 + r3
w22 = w22 + r0

d0 = w00 + w01
c0 = d0 + w02
d1 = w10 + w11
c1 = d1 + w12
d2 = w20 + w21
d2 = d2 + w22
c2 = d2 + r4
