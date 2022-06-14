# Random Probing Security checker

To get the coefficient:

    ./exec.sh 'gadget' 'max coefficient'

To generate the ISW multiplications, inside ./gadgets/:

    ./isw_mul_generator.py 'num shares' > 'output file'

To directly get the coefficient for a ISW multiplication:

    ./exec.sh <(gadgets/isw_mul_generator.py 'num shares')  'max coefficient'


Note: In the .sage files, each assignment causes probes.
