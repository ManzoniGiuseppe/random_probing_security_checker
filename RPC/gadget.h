#ifndef _GADGET_H_
#define _GADGET_H_


#include "bdd.h"


extern int gadget_probeMulteplicity[NUM_PROBES];

void gadget_fn(bdd_t x[NUM_TOT_INS], bdd_t ret[NUM_TOT_OUTS]);



#endif // _GADGET_H_


