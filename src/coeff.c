
#include "coeff.h"

// useful def
#define MAX(x, y)  ((x) > (y) ? (x) : (y))


static double calculateF(coeff_t coeffs, double p){
  double ret = 0.0;
  double pi;
  shift_t i;
  for(pi = 1.0, i = 0; i <= MAX_COEFF; i++, pi *= p){
    ret += coeffs.values[i] * pi;
  }
  return MIN(1.0, ret); // f is a probability, so it's in [0,1]
}


coeff_t coeff_zero(void){
  coeff_t ret;
  for(shift_t i = 0; i <= MAX_COEFF; i++)
    ret.values[i] = 0.0;
  ret.approximated = 0;
  return ret;
}


coeff_t coeff_cmp(bool want_max, coeff_t v1, coeff_t v2, bool *ret_used1, bool *ret_used2){
  bool want_min = !want_max;

  // check the coefficients
  bool all_ge = 1;
  bool all_le = 1;
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    if(v1.values[i] < v2.values[i]) all_ge = 0;
    if(v1.values[i] > v2.values[i]) all_le = 0;
  }

  if((all_ge && want_max) || (all_le && want_min)){
    if(ret_used1) *ret_used1 = 1;
    if(ret_used2) *ret_used2 = 0;
    return v1;
  }
  if((all_le && want_max) || (all_ge && want_min)){
    if(ret_used1) *ret_used1 = 0;
    if(ret_used2) *ret_used2 = 1;
    return v2;
  }

  // check again by sempling the functions, to see if all the coeff are actually expressed
  all_ge = all_le = 1;
  for(double p = 0.0; p <= 1.0; p+=FN_CMP_STEP){
    double cf_v1 = calculateF(v1, p);
    double cf_v2 = calculateF(v2, p);
    if(cf_v1 < cf_v2) all_ge = 0;
    if(cf_v1 > cf_v2) all_le = 0;
  }

  if((all_ge && want_max) || (all_le && want_min)){
    if(ret_used1) *ret_used1 = 1;
    if(ret_used2) *ret_used2 = 0;
    return v1;
  }
  if((all_le && want_max) || (all_ge && want_min)){
    if(ret_used1) *ret_used1 = 0;
    if(ret_used2) *ret_used2 = 1;
    return v2;
  }

  // mixed
  if(ret_used1) *ret_used1 = 1;
  if(ret_used2) *ret_used2 = 1;
  coeff_t ret;
  for(shift_t i = 0; i <= MAX_COEFF; i++)
    ret.values[i] = MAX(v1.values[i], v2.values[i]); // can't return a f < actual, so approximate it by using the maximum coefficients.
  ret.approximated = 1;
  return ret;
}

coeff_t coeff_add(coeff_t v1, coeff_t v2){
  coeff_t ret;
  for(shift_t i = 0; i <= MAX_COEFF; i++)
    ret.values[i] = v1.values[i] + v2.values[i];
  ret.approximated = v1.approximated || v2.approximated;
  return ret;
}

coeff_t coeff_times(coeff_t v1, double c){
  for(shift_t i = 0; i <= MAX_COEFF; i++)
    v1.values[i] *= c;
  return v1;
}
