//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "gadget.h"
#include "mem.h"



//---  get basic info on the gadget



typedef struct {
  wire_t d;
  wire_t numIns;
  wire_t numRnds;
  wire_t numOuts;
  wire_t numOperations;  // numProbes <= numTotIns + numOperations
} basicGadgetInfo_t;

static basicGadgetInfo_t basicGadgetInfo(gadget_raw_t r);


static gadget_wireId_t basicGadgetInfo_rnd(void *state){
  basicGadgetInfo_t *i = state;
  i->numRnds++;
  return 0;
}
static gadget_wireId_t basicGadgetInfo_in(void *state, wire_t numIn, wire_t numShare){
  basicGadgetInfo_t *i = state;
  i->numIns = MAX(i->numIns, numIn+1);
  i->d = MAX(i->d, numShare+1);
  return 0;
}
static gadget_wireId_t basicGadgetInfo_not(void *state, __attribute__((unused)) gadget_wireId_t v){
  basicGadgetInfo_t *i = state;
  i->numOperations++;
  return 0;
}
static gadget_wireId_t basicGadgetInfo_binary(void *state, __attribute__((unused)) int operation, __attribute__((unused)) gadget_wireId_t v0, __attribute__((unused)) gadget_wireId_t v1){
  basicGadgetInfo_t *i = state;
  i->numOperations++;
  return 0;
}
static void basicGadgetInfo_out(void *state, __attribute__((unused)) gadget_wireId_t v, wire_t numOut, wire_t numShare){
  basicGadgetInfo_t *i = state;
  i->numOuts = MAX(i->numOuts, numOut+1);
  i->d = MAX(i->d, numShare+1);
}
static basicGadgetInfo_t basicGadgetInfo(gadget_raw_t r){
  basicGadgetInfo_t ret = (basicGadgetInfo_t){
    .d = 0,
    .numIns = 0,
    .numRnds = 0,
    .numOuts = 0,
    .numOperations = 0
  };

  gadget_fnBuilder_t builder = (gadget_fnBuilder_t) {
    .state = &ret,
    .rnd = basicGadgetInfo_rnd,
    .in = basicGadgetInfo_in,
    .not = basicGadgetInfo_not,
    .binary = basicGadgetInfo_binary,
    .out = basicGadgetInfo_out
  };

  r.fn(r.fn_info, builder);

  return ret;
}



//---- gadget_fromRaw


typedef struct {
  int operation;
  wire_t inWire[2];
  int mult;
} fnGenerator_op_t;

typedef struct{
  wire_t d;
  wire_t numIns;
  wire_t numOuts;
  wire_t numAllocRnds;

  wire_t numOps; // = numTotIns
  fnGenerator_op_t *op; // op[numTotIns + numOperations]
  wire_t *out2op;  // out2op[d * numOuts]
} fnGenerator_t;

static fnGenerator_t fnGenerator_alloc(basicGadgetInfo_t i){
  wire_t numTotIns = i.d * i.numIns + i.numRnds;
  fnGenerator_t ret = (fnGenerator_t){
    .d = i.d,
    .numIns = i.numIns,
    .numOuts = i.numOuts,
    .numAllocRnds = 0,

    .numOps = numTotIns,
    .op = mem_calloc(sizeof(fnGenerator_op_t), numTotIns + i.numOperations, "fnGenerator_alloc's op"),
    .out2op = mem_calloc(sizeof(wire_t), i.d * i.numOuts, "fnGenerator_alloc's out2op")
  };

  for(wire_t j = 0; j < numTotIns; j++)
    ret.op[j] = (fnGenerator_op_t){
      .operation = GADGET_IN,
      .inWire = {j, 0},
      .mult = -1
    };
  return ret;
}
static void fnGenerator_free(__attribute__((unused)) fnGenerator_t gen){
  mem_free(gen.op);
  mem_free(gen.out2op);
}


static gadget_wireId_t gen_rnd(void *state){
  fnGenerator_t *gen = state;

  return gen->numIns * gen->d + (gen->numAllocRnds++);
}
static gadget_wireId_t gen_in(void *state, wire_t numIn, wire_t numShare){
  fnGenerator_t *gen = state;
  return numShare + gen->d * numIn;
}

static gadget_wireId_t gen_not(void *state, gadget_wireId_t v){
  fnGenerator_t *gen = state;
  gen->op[v].mult += 2;

  wire_t op = gen->numOps++;
  gen->op[op] = (fnGenerator_op_t){
    .operation = GADGET_NOT,
    .inWire = {v, 0},
    .mult = -1
  };
  return op;
}
static gadget_wireId_t gen_binary(void *state, int operation, gadget_wireId_t v0, gadget_wireId_t v1){
  fnGenerator_t *gen = state;
  gen->op[v0].mult += 2;
  gen->op[v1].mult += 2;

  wire_t op = gen->numOps++;
  gen->op[op] = (fnGenerator_op_t){
    .operation = operation,
    .inWire = {v0, v1},
    .mult = -1
  };
  return op;
}
static void gen_out(void *state, gadget_wireId_t v, wire_t numOut, wire_t numShare){
  fnGenerator_t *gen = state;
  gen->op[v].mult += 1; // for the input of the copy gadget, but not the output itself

  gen->out2op[numOut * gen->d + numShare] = v;
}




gadget_t *gadget_fromRaw(gadget_raw_t raw){
  basicGadgetInfo_t i = basicGadgetInfo(raw);
  wire_t d = i.d;
  wire_t numIns = i.numIns;
  wire_t numOuts = i.numOuts;
  wire_t numRnds = i.numRnds;

  if(numIns * d + numRnds > MAX_NUM_TOT_INS) FAIL("Gadget: More inputs (%d) than supported (%d)", (int) numIns * d + numRnds, (int) MAX_NUM_TOT_INS);

  fnGenerator_t gen = fnGenerator_alloc(i);

  gadget_fnBuilder_t builder = (gadget_fnBuilder_t){
    .state = &gen,
    .rnd = gen_rnd,
    .in = gen_in,
    .not = gen_not,
    .binary = gen_binary,
    .out = gen_out
  };

  raw.fn(raw.fn_info, builder);

  wire_t numProbes = 0;
  wire_t numTotProbes = 0;
  for(unsigned j = 0; j < gen.numOps; j++){
    if(gen.op[j].mult > 0){
       numProbes++;
       numTotProbes += gen.op[j].mult;
    }
  }

  int *probeMulteplicity = mem_calloc(sizeof(int), numProbes, "gadget_fromRaw's final probeMulteplicity");
  gadget_fnOperation_t *op = mem_calloc(sizeof(gadget_fnOperation_t), d*numOuts+numProbes, "gadget_fromRaw's final op");
  int *opRelocator = mem_calloc(sizeof(int), gen.numOps, "gadget_fromRaw's op relocator");

  int numOp = 0;
  for(wire_t j = 0; j < gen.numOps; j++){
    if(gen.op[j].mult <= 0) continue;

    op[numOp].operation = gen.op[j].operation;
    if(op[numOp].operation == GADGET_IN)
      op[numOp].inWire[0] = gen.op[j].inWire[0];
    else if(op[numOp].operation == GADGET_NOT)
      op[numOp].inWire[0] = opRelocator[gen.op[j].inWire[0]];
    else{
      op[numOp].inWire[0] = opRelocator[gen.op[j].inWire[0]];
      op[numOp].inWire[1] = opRelocator[gen.op[j].inWire[1]];
    }
    op[numOp].outWire = numOp + d*numOuts;
    probeMulteplicity[numOp] = gen.op[j].mult;
    opRelocator[j] = op[numOp++].outWire;
  }
  for(wire_t o = 0; o < d*numOuts; o++){
    wire_t j = gen.out2op[o];
    op[numProbes+o].operation = gen.op[j].operation;
    if(op[numProbes+o].operation == GADGET_IN)
      op[numProbes+o].inWire[0] = gen.op[j].inWire[0];
    else if(op[numProbes+o].operation == GADGET_NOT)
      op[numProbes+o].inWire[0] = opRelocator[gen.op[j].inWire[0]];
    else{
      op[numProbes+o].inWire[0] = opRelocator[gen.op[j].inWire[0]];
      op[numProbes+o].inWire[1] = opRelocator[gen.op[j].inWire[1]];
    }
    op[numProbes+o].outWire = o;
  }

  mem_free(opRelocator);
  fnGenerator_free(gen);

  gadget_t *ret = mem_calloc(sizeof(gadget_t), 1, "gadget_fromRaw's gadget_t");
  *ret = (gadget_t){
    .d = d,
    .numIns = numIns,
    .numRnds = numRnds,
    .numTotIns = d*numIns+numRnds,
    .numOuts = numOuts,
    .numProbes = numProbes,
    .numTotOuts = d*numOuts+numProbes,
    .numTotProbes = numTotProbes,
    .probeMulteplicity = probeMulteplicity,
    .op = op
  };
  return ret;
}

void gadget_free(gadget_t *g){
  mem_free(g->probeMulteplicity);
  mem_free(g->op);
  mem_free(g);
}



//--- gadget_printInfo


static void printRet(wire_t out, wire_t d, wire_t numOuts, int *mul){
  if(out >= d*numOuts){
    wire_t probe = out - d*numOuts;
    if(mul[probe] > 1)
      printf("[%dx]probe%d", mul[probe], probe);
    else
      printf("probe%d", probe);
  }else
    printf("out%dsh%d", out/d, out%d);
}

static void printOut(wire_t out, wire_t d, wire_t numOuts){
  if(out >= d*numOuts)
    printf("probe%d", out - d*numOuts);
  else
    printf("out%dsh%d", out/d, out%d);
}
static void printIn(wire_t in, wire_t d, wire_t numIns){
  if(in >= d*numIns)
    printf("rnd%d", in - d*numIns);
  else
    printf("in%dsh%d", in/d, in%d);
}

void gadget_print(gadget_t *g){
  printf("D=%d\n", g->d);
  printf("NUM_INS=%d\n", g->numIns);
  printf("NUM_MASKED_INS=%d\n", g->d * g->numIns);
  printf("NUM_RNDS=%d\n", g->numRnds);
  printf("NUM_TOT_INS=%d\n", g->numTotIns);
  printf("NUM_OUTS=%d\n", g->numOuts);
  printf("NUM_MASKED_OUTS=%d\n", g->d * g->numOuts);
  printf("NUM_PROBES=%d\n", g->numProbes);
  printf("NUM_TOT_OUTS=%d\n", g->numTotOuts);
  printf("NUM_TOT_PROBES=%d\n", g->numTotProbes);
  for(wire_t i = 0; i < g->numTotOuts; i++){
    printf("  ");
    printRet(g->op[i].outWire, g->d, g->numOuts, g->probeMulteplicity);
    printf(" = ");
    if(g->op[i].operation == GADGET_IN){
      printIn(g->op[i].inWire[0], g->d, g->numIns);
    }else if(g->op[i].operation == GADGET_NOT){
      printf(" ! ");
      printOut(g->op[i].inWire[0], g->d, g->numOuts);
    }else{
      printOut(g->op[i].inWire[0], g->d, g->numOuts);
      switch(g->op[i].operation){
        case GADGET_BIN_OR: printf(" OR "); break;
        case GADGET_BIN_IMPLY: printf(" IMPLY "); break;
        case GADGET_BIN_XOR: printf(" XOR "); break;
        case GADGET_BIN_AND: printf(" AND "); break;
        case GADGET_BIN_NOR: printf(" NOR "); break;
        case GADGET_BIN_NIMPLY: printf(" NIMPLY "); break;
        case GADGET_BIN_XNOR: printf(" XNOR "); break;
        case GADGET_BIN_NAND: printf(" NAND "); break;
      }
      printOut(g->op[i].inWire[1], g->d, g->numOuts);
    }
    printf("\n");
  }
}
