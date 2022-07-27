#!/bin/bash

echo "refresh_ring 10 1"
diff <(./exec.sh -s gadgets/refresh_ring.sage -c 10 --rps --rpc=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1.000000 21.000000 174.000000 736.000000 1779.000000 2748.000000 2895.000000 2148.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 0.500000 10.500000 100.500000 564.500000 1779.000000 2748.000000 2895.000000 2148.000000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 111.000000 702.000000 2177.000000 4293.000000 5997.000000 6243.000000 4947.000000 2992.000000
RPC: coeffs of Mgm:   0.000000 0.000000 4.500000 105.000000 702.000000 2177.000000 4293.000000 5997.000000 6243.000000 4947.000000 2992.000000
RPC: coeffs of Mteo:  0.000000 0.000000 0.750000 7.125000 31.500000 86.343750 164.671875 231.703125 247.781250 203.531250 127.875000
EOF
)

echo "vrapsPaper_add_v1 6 1"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v1.sage -c 6 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 4.000000 186.000000 3978.000000 51842.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 2.000000 93.000000 2070.000000 29470.000000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 264.000000 5661.000000 76118.000000 696667.000000
RPC: coeffs of Mgm:   0.000000 0.000000 6.000000 250.000000 5238.000000 71606.000000 680203.000000
EOF
)

echo "vrapsPaper_add_v1 10 (RPS)"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v1.sage -c 10 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 4.000000 186.000000 3978.000000 51842.000000 463866.000000 3054981.000000 15500672.000000 62520675.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 2.000000 93.000000 2070.000000 29470.000000 298464.000000 2247729.000000 12893016.000000 57294531.000000
EOF
)

echo "vrapsPaper_add_v2 6 1"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v2.sage -c 6 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 4.000000 177.000000 3711.000000 48635.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 2.000000 88.500000 1927.500000 27406.000000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 256.000000 5583.000000 77340.000000 720305.000000
RPC: coeffs of Mgm:   0.000000 0.000000 6.000000 240.000000 5056.000000 71556.000000 702693.000000
EOF
)

echo "vrapsPaper_add_v2 10 (RPS)"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v2.sage -c 10 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 4.000000 177.000000 3711.000000 48635.000000 445914.000000 3040122.000000 15986207.000000 66390357.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 2.000000 88.500000 1927.500000 27406.000000 282099.000000 2193462.000000 13108074.000000 60519834.000000
EOF
)

echo "vrapsPaper_add_v3 6 1"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v3.sage -c 6 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 2.000000 78.000000 1497.000000 18757.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 1.000000 39.000000 777.000000 10496.500000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 224.000000 4423.000000 59776.000000 580731.000000
RPC: coeffs of Mgm:   0.000000 0.000000 6.000000 224.000000 4338.000000 57761.000000 568805.000000
EOF
)

echo "vrapsPaper_add_v3 10 (RPS)"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v3.sage -c 10 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 2.000000 78.000000 1497.000000 18757.000000 172077.000000 1228674.000000 7068743.000000 33288438.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 1.000000 39.000000 777.000000 10496.500000 106779.000000 856300.500000 5522623.000000 28832418.000000
EOF
)

echo "vrapsPaper_add_v4 6 1"
diff <(./exec.sh -s gadgets/vrapsPaper_add_v4.sage -c 6 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 2.000000 84.000000 1710.000000 22328.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 1.000000 42.000000 891.000000 12625.500000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 248.000000 5094.000000 68278.000000 641544.000000
RPC: coeffs of Mgm:   0.000000 0.000000 6.000000 248.000000 5028.000000 66831.500000 634564.500000
EOF
)

echo "add_test_1 6 1"
diff <(./exec.sh -s gadgets/add_test_1.sage -c 6 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 4.000000 177.000000 3756.000000 50048.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 2.000000 88.500000 1950.000000 28216.000000
RPC: coeffs of M0:    0.000000 0.000000 21.000000 715.000000 11875.000000 127660.000000 977348.000000
RPC: coeffs of Mgm:   0.000000 0.000000 21.000000 708.500000 11667.500000 125409.000000 970108.000000
EOF
)

echo "add_test_2 6 1"
diff <(./exec.sh -s gadgets/add_test_2.sage -c 6 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 2.000000 70.000000 1198.000000 13362.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 1.000000 35.000000 618.500000 7382.500000
RPC: coeffs of M0:    0.000000 0.000000 15.000000 516.000000 8552.000000 91404.000000 706681.000000
RPC: coeffs of Mgm:   0.000000 0.000000 13.500000 477.000000 8105.500000 88520.000000 697321.000000
EOF
)

echo "vrapsPaper_copy 10 1"
diff <(./exec.sh -s gadgets/vrapsPaper_copy.sage -c 10 --rps --rpc=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 27.000000 891.000000 13554.000000 126954.000000 826236.000000 4001787.000000 15095134.000000 45820470.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 13.500000 486.000000 8532.000000 94014.000000 705708.000000 3768507.000000 14908510.000000 45820470.000000
RPC: coeffs of M0:    0.000000 0.000000 33.000000 1137.000000 16812.000000 145288.000000 852472.000000 3732534.000000 12981389.000000 37342867.000000 91195272.000000
RPC: coeffs of Mgm:   0.000000 0.000000 30.000000 1089.000000 16572.000000 144904.000000 852472.000000 3732534.000000 12981389.000000 37342867.000000 91195272.000000
RPC: coeffs of Mteo:  0.000000 0.000000 6.750000 118.125000 1030.500000 5977.125000 25922.156250 89570.062500 256427.578125 624204.679688 1315244.625000
EOF
)

echo "vrapsPaper_mul 4 1"
diff <(./exec.sh -s gadgets/vrapsPaper_mul.sage -c 4 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1091.000000 95997.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 300.062500 31758.375000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 1953.000000 154973.000000
RPC: coeffs of Mgm:   0.000000 0.000000 6.000000 982.312500 74597.500000
EOF
)

echo "vrapsPaper_mul 5 (RPS)"
diff <(./exec.sh -s gadgets/vrapsPaper_mul.sage -c 5 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1091.000000 95997.000000 4141455.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 300.062500 31758.375000 1661144.812500
EOF
)

echo "isw_mul_2 10 1"
diff <(./exec.sh -s <(gadgets/isw_mul_generator.py 2) -c 10 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 51.000000 754.000000 4827.000000 18875.000000 52994.000000 115520.000000 203176.000000 293844.000000 352702.000000
RPS: coeffs of Mgm:   0.000000 0.000000 17.875000 425.375000 4011.250000 18875.000000 52994.000000 115520.000000 203176.000000 293844.000000 352702.000000
RPC: coeffs of M0:    0.000000 4.000000 131.000000 1173.000000 5810.000000 20230.000000 54215.000000 116269.000000 203489.000000 293930.000000 352716.000000
RPC: coeffs of Mgm:   0.000000 3.500000 131.000000 1173.000000 5810.000000 20230.000000 54215.000000 116269.000000 203489.000000 293930.000000 352716.000000
EOF
)

echo "isw_mul_3 8 1"
diff <(./exec.sh -s <(gadgets/isw_mul_generator.py 3) -c 8 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1178.000000 53135.000000 1149033.000000 15844475.000000 157122283.000000 1202023660.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 313.437500 19959.562500 613278.312500 11353203.562500 137899674.125000 1169122186.250000
RPC: coeffs of M0:    0.000000 0.000000 428.000000 17351.000000 327328.000000 3954080.000000 35738768.000000 263430852.000000 1651133549.000000
RPC: coeffs of Mgm:   0.000000 0.000000 426.500000 17339.500000 327328.000000 3954080.000000 35738768.000000 263430852.000000 1651133549.000000
EOF
)

echo "isw_mul_3 10 (RPS)"
diff <(./exec.sh -s <(gadgets/isw_mul_generator.py 3) -c 10 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1178.000000 53135.000000 1149033.000000 15844475.000000 157122283.000000 1202023660.000000 7446862403.0 38724322692.0000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 313.437500 19959.562500 613278.312500 11353203.562500 137899674.125000 1169122186.250000 7446862403.0 38724322692.0000
EOF
)

echo "isw_mul_4 4 1"
diff <(./exec.sh -s <(gadgets/isw_mul_generator.py 4) -c 4 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 0.000000 34385.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 0.000000 7607.156250
RPC: coeffs of M0:    0.000000 0.000000 1459.000000 112820.000000 4271673.000000
RPC: coeffs of Mgm:   0.000000 0.000000 1458.500000 112799.000000 4271343.500000
EOF
)

echo "isw_mul_4 5 (RPS)"
diff <(./exec.sh -s <(gadgets/isw_mul_generator.py 4) -c 5 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 0.000000 34385.000000 3077435.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 0.000000 7607.156250 925163.593750
EOF
)

echo "otpoePaper_refresh 10 1"
diff <(./exec.sh -s gadgets/otpoePaper_refresh.sage -c 10 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1.000000 7.000000 21.000000 35.000000 35.000000 21.000000 7.000000 1.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 0.500000 3.500000 13.500000 35.000000 35.000000 21.000000 7.000000 1.000000
RPC: coeffs of M0:    0.000000 0.000000 9.000000 58.000000 138.000000 196.000000 182.000000 112.000000 44.000000 10.000000 1.000000
RPC: coeffs of Mgm:   0.000000 0.000000 9.000000 58.000000 138.000000 196.000000 182.000000 112.000000 44.000000 10.000000 1.000000
EOF
)

echo "otpoePaper_add 10 1"
diff <(./exec.sh -s gadgets/otpoePaper_add.sage -c 10 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 4.000000 120.000000 1636.000000 13274.000000 72580.000000 287344.000000 860984.000000 2010862.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 2.000000 60.000000 868.000000 8048.000000 51926.000000 240570.000000 811298.000000 2010862.000000
RPC: coeffs of M0:    0.000000 0.000000 6.000000 272.000000 4330.000000 34898.000000 170109.000000 571508.000000 1465596.000000 3036852.000000 5245902.000000
RPC: coeffs of Mgm:   0.000000 0.000000 6.000000 244.000000 4134.500000 34898.000000 170109.000000 571508.000000 1465596.000000 3036852.000000 5245902.000000
EOF
)

echo "otpoePaper_copy 10 1"
diff <(./exec.sh -s gadgets/otpoePaper_copy.sage -c 10 --rps --rpcIs=1 --rpcSum=1 | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 27.000000 459.000000 3699.000000 18792.000000 67509.000000 182331.000000 384238.000000 647141.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 13.500000 270.000000 2632.500000 15957.000000 64471.500000 182331.000000 384238.000000 647141.000000
RPC: coeffs of M0:    0.000000 0.000000 72.000000 1044.000000 7050.000000 30510.000000 96917.000000 241267.000000 487491.000000 815669.000000 1143474.000000
RPC: coeffs of Mgm:   0.000000 0.000000 72.000000 1044.000000 7050.000000 30510.000000 96917.000000 241267.000000 487491.000000 815669.000000 1143474.000000
EOF
)

echo "wrong_isw_mul_2 5 (RPS)"
diff <(./exec.sh -s <(gadgets/wrong_isw_mul_generator.py 2) -c 5 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 51.000000 754.000000 4827.000000 18875.0
RPS: coeffs of Mgm:   0.000000 0.000000 17.875000 425.375000 4011.250000 18875.0
EOF
)

echo "wrong_isw_mul_3 5 (RPS)"
diff <(./exec.sh -s <(gadgets/wrong_isw_mul_generator.py 3) -c 5 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 1251.000000 56860.000000 1223237.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 324.812500 20901.375000 650916.875000
EOF
)

echo "wrong_isw_mul_4 5 (RPS)"
diff <(./exec.sh -s <(gadgets/wrong_isw_mul_generator.py 4) -c 5 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 0.000000 0.000000 35989.000000 3252053.000000
RPS: coeffs of Mgm:   0.000000 0.000000 0.000000 0.000000 7813.406250 956408.000000
EOF
)

echo "wrong_isw2_mul_from_vraps_readme 5 (RPS)"
diff <(./exec.sh -s gadgets/wrong_isw2_mul_from_vraps_readme.sage -c 5 --rps | grep '^RP[SC]: coeffs of ' ) <(cat << EOF
RPS: coeffs of M0:    0.000000 0.000000 51.000000 754.000000 4827.000000 18875.000000
RPS: coeffs of Mgm:   0.000000 0.000000 17.500000 411.500000 3907.750000 18875.000000
EOF
)