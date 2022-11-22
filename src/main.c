//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "license.h"
#include "help.h"
#include "calc.h"
#include "gadget.h"
#include "parserSage.h"
#include "multithread.h"


// TODO  otpoePaper_mul.py__5  rpc* seem to fail
// TODO  why  RPS\_COR2 != RPS\_COR3 in the add with only xor.  near exact ratio of 1.5 everywhere. Why.

void printCoeff(const char* what, double *coeffs, wire_t maxCoeff){
  printf("%s:", what);
  for(wire_t i = 0; i <= maxCoeff; i++){
    printf(" %f", coeffs[i]);
  }
  printf("\n");
}

// output the info.
int main(int argc, char **argv){
  int maxCoeff = -1;
  int t = -1;
  char *sage = NULL;
  bool isRpc = 0;

  bool isRpsCor1 = 0;
  bool isRpsCor2 = 0;
  bool isRpsCor3 = 0;
  bool isRpsVraps = 0;

  bool isRpcCor1 = 0;
  bool isRpcCor2 = 0;
  bool isRpcVraps = 0;

  bool printGadget = 0;

  for(int i = 1; i < argc; i++){
    if(strcmp(argv[i], "--sage") == 0){
      if(sage) FAIL("Input file already specified.\n")
      sage = argv[++i];
    }else if(strcmp(argv[i], "-c") == 0){
      maxCoeff = atoi(argv[++i]);
    }else if(strcmp(argv[i], "-t") == 0){
      t = atoi(argv[++i]);
    }else if(strcmp(argv[i], "--rpsCor1") == 0){
      isRpsCor1 = 1;
    }else if(strcmp(argv[i], "--rpsCor2") == 0){
      isRpsCor2 = 1;
    }else if(strcmp(argv[i], "--rpsCor3") == 0){
      isRpsCor3 = 1;
    }else if(strcmp(argv[i], "--rpsVraps") == 0){
      isRpsVraps = 1;
    }else if(strcmp(argv[i], "--rpcCor1") == 0){
      isRpcCor1 = 1;
      isRpc = 1;
    }else if(strcmp(argv[i], "--rpcCor2") == 0){
      isRpcCor2 = 1;
      isRpc = 1;
    }else if(strcmp(argv[i], "--rpcVraps") == 0){
      isRpcVraps = 1;
      isRpc = 1;
    }else if(strcmp(argv[i], "--printGadget") == 0){
      printGadget = 1;
    }else if(strcmp(argv[i], "--license") == 0){
      printf("%s", license);
      return 0;
    }else if(strcmp(argv[i], "--help") == 0){
      printf("%s", help);
      return 0;
    }else{
      FAIL("unrecognized option: %s\n", argv[i]);
    }
  }

  if(!isRpc && t >= 0) FAIL("Parameter T should only be specified with --rpc*\n")
  if(isRpc && t < 0) FAIL("Parameter T must be specified with --rpc*\n")
  if(!sage) FAIL("Missing input file.\n")

  gadget_t *g;
  {
    printf("Parse sage to raw gadget...\n");
    gadget_raw_t r = parserSage_parse(sage);
    printf("Raw gadget to gadget...\n");
    g = gadget_fromRaw(r);
    r.free(r.fn_info);
  }

  if(maxCoeff < 0) maxCoeff = g->numTotProbes;
  if(t >= g->d) FAIL("Parameter T should only be >= 0 and < of the number of shares of the gadget\n")

  if(printGadget){
    gadget_print(g);
  }

  if(isRpsCor1){
    double coeffs[g->numTotProbes+1];
    calc_rpsCor1(g, maxCoeff, coeffs);
    printCoeff("rpsCor1-ret", coeffs, g->numTotProbes);
  }
  if(isRpsCor2){
    double coeffs[g->numTotProbes+1];
    calc_rpsCor2(g, maxCoeff, coeffs);
    printCoeff("rpsCor2-ret", coeffs, g->numTotProbes);
  }
  if(isRpsCor3){
    double coeffs[g->numTotProbes+1];
    calc_rpsCor3(g, maxCoeff, coeffs);
    printCoeff("rpsCor3-ret", coeffs, g->numTotProbes);
  }
  if(isRpsVraps){
    double coeffs[g->numTotProbes+1];
    calc_rpsVraps(g, maxCoeff, coeffs);
    printCoeff("rpsVraps-ret", coeffs, g->numTotProbes);
  }

  if(isRpcCor1){
    double coeffs[g->numTotProbes+1];
    calc_rpcCor1(g, maxCoeff, t, coeffs);
    printCoeff("rpcCor1-ret", coeffs, g->numTotProbes);
  }
  if(isRpcCor2){
    double coeffs[g->numTotProbes+1];
    calc_rpcCor2(g, maxCoeff, t, coeffs);
    printCoeff("rpcCor2-ret", coeffs, g->numTotProbes);
  }
  if(isRpcVraps){
    double coeffs[g->numTotProbes+1];
    calc_rpcVraps(g, maxCoeff, t, coeffs);
    printCoeff("rpcVraps-ret", coeffs, g->numTotProbes);
  }

  gadget_free(g);
  return 0;
}
