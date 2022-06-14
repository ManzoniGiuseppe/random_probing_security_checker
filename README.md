# Random Probing Security checker

To get the coefficient:

    ./exec.sh 'gadget' 'max coefficient'

To generate the ISW multiplications, inside ./gadgets/:

    ./isw_mul_generator.py 'num shares' > 'output file'

To directly get the coefficient for a ISW multiplication (at least in bash):

    ./exec.sh <(gadgets/isw_mul_generator.py 'num shares')  'max coefficient'


Note: In the .sage files:
 - all assignments must have exactly one operation (+ or *)
 - no input must be used directly as output
 - no input must be written to
