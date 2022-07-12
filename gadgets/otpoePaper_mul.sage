#SHARES 3
#IN a b
#RANDOMS r1 r2 r3 r4 r5 r6 r7 r8 r11 r12 r13 r21 r22 r23 r31 r32 r33
#OUT d

i11 = r1 + b0
i12 = r2 + b1
i13 = r1 + r2
i13 = i13 + b2

i21 = r3 + b0
i22 = r4 + b1
i23 = r3 + r4
i23 = i23 + b2

i31 = r5 + b0
i32 = r6 + b1
i33 = r5 + r6
i33 = i33 + b2

p1 = r7 + a0
p2 = r8 + a1
p3 = r7 + r8
p3 = p3 + a2

w11 = p1 * i11
w11 = w11 + r11
w12 = p1 * i12
w12 = w12 + r12
w13 = p1 * i13
w13 = w13 + r13
w14 = r11 + r21
w14 = w14 + r31

w21 = p2 * i21
w21 = w21 + r21
w22 = p2 * i22
w22 = w22 + r22
w23 = p2 * i23
w23 = w23 + r23
w24 = r12 + r22
w24 = w24 + r32

w31 = p3 * i31
w31 = w31 + r31
w32 = p3 * i32
w32 = w32 + r32
w33 = p3 * i33
w33 = w33 + r33
w34 = r13 + r23
w34 = w34 + r33

c0 = w11 + w12
c0 = c0 + w13
d0 = c0 + w14

c1 = w21 + w22
c1 = c1 + w23
d1 = c1 + w24

c2 = w31 + w32
c2 = c2 + w33
d2 = c2 + w34
