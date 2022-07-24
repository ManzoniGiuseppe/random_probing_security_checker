#ifndef _COEFF_H_
#define _COEFF_H_


#include "types.h"

typedef struct {
  bool approximated;
  double values[MAX_COEFF+1];
} coeff_t;


coeff_t coeff_zero(void);
coeff_t coeff_add(coeff_t v1, coeff_t v2);

coeff_t coeff_cmp(bool want_max, coeff_t v1, coeff_t v2, bool *ret_used1, bool *ret_used2);
#define coeff_max_and_usage(v1, v2, ret_used_v1, ret_used_v2)  (coeff_cmp(1, (v1), (v2), (ret_used_v1), (ret_used_v2)))
#define coeff_min_and_usage(v1, v2, ret_used_v1, ret_used_v2)  (coeff_cmp(0, (v1), (v2), (ret_used_v1), (ret_used_v2)))
#define coeff_max(v1, v2)  coeff_max_and_usage(v1, v2, NULL, NULL)
#define coeff_min(v1, v2)  coeff_min_and_usage(v1, v2, NULL, NULL)

#endif // _COEFF_H_
