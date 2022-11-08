//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _GADGET_H_
#define _GADGET_H_

#include "bdd.h"
#include "types.h"


typedef unsigned gadget_wireId_t;

#define GADGET_BIN_OR       0
#define GADGET_BIN_IMPLY    1
#define GADGET_BIN_XOR      2
#define GADGET_BIN_AND      3
#define GADGET_BIN_NOR      4
#define GADGET_BIN_NIMPLY   5
#define GADGET_BIN_XNOR     6
#define GADGET_BIN_NAND     7
#define GADGET_NOT   8
#define GADGET_IN    9

// assumes:
//  - all shares of all outs must be set
typedef struct {
  void *state;

  gadget_wireId_t (*rnd)(void *state);
  gadget_wireId_t (*in)(void *state, int numIn, int numShare);

  gadget_wireId_t (*not)(void *state, gadget_wireId_t v);
  gadget_wireId_t (*binary)(void *state, int operation /*GADGET_BIN_**/, gadget_wireId_t v0, gadget_wireId_t v1);

  void (*out)(void *state, gadget_wireId_t v, int numOut, int numShare);
} gadget_fnBuilder_t;

typedef struct {
  void *fn_info;
  T__THREAD_SAFE void (*fn)(void *fn_info, gadget_fnBuilder_t builder);
  void (*free)(void *fn_info);
} gadget_raw_t;




typedef struct {
  int operation; // GADGET_*
  wire_t inWire[2];

  wire_t outWire;
} gadget_fnOperation_t;

typedef struct {
  wire_t d;

  wire_t numIns;
  wire_t numRnds;
  wire_t numTotIns;

  wire_t numOuts;
  wire_t numProbes;
  wire_t numTotOuts;

  wire_t numTotProbes; // = \sum_i probeMulteplicity[i]
  int *probeMulteplicity; // probeMulteplicity[numProbes]

  gadget_fnOperation_t *op; // it contains numTotOuts operations.
} gadget_t;


gadget_t *gadget_fromRaw(gadget_raw_t raw);
T__THREAD_SAFE void gadget_print(gadget_t *g);
void gadget_free(gadget_t *g);


#endif // _GADGET_H_


