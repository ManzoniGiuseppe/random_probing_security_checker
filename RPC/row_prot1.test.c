#include <stdio.h>
#include <stdlib.h>

#include "row.h"

#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }


int main(){
  row_t v = row_first();

  // D=3
  // T=2
  // NUM_PROBES=3
  // MAX_COEFF=2

  int outs[] = {0,1,2,3,4,5,6};
  int probes[] = {0,1,2,3,4,5,6};

  for(int i = 0; i < sizeof(probes)/sizeof(int); i++){
    for(int j = 0; j < sizeof(outs)/sizeof(int); j++){
      if( i == 0 && j == 0) continue;
      if(!row_tryNextProbeAndOut(& v)) FAIL("try failed at %d,%d\n", i,j)
      if(v.values[0] != (probes[i]<<D) + outs[j]) FAIL("next failed at %d,%d: returned %x\n", i,j, v.values[0])
    }
  }
  if(row_tryNextProbeAndOut(& v)) FAIL("try succeded one time too many, returned %d\n", v.values[0])

  printf("Success!\n");
}
