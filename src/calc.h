#ifndef _CALC_H_
#define _CALC_H_

#include "coeff.h"

// need to fill up rowTransform before calling this.

coeff_t calc_rpsIs(void);
coeff_t calc_rpsSum(void);
coeff_t calc_rpsTeo(void);
coeff_t calc_rpcIs(void);
coeff_t calc_rpcSum(void);
coeff_t calc_rpcMix(void);
coeff_t calc_rpcTeo(void);


#endif // _CALC_H_
