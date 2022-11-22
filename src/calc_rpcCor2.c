//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <math.h>
#include "calc.h"


void calc_rpcCor2(gadget_t *g, wire_t maxCoeff, wire_t t, double *ret_coeffs){
  double sdMax = 1 - ldexp(1.0, -g->numIns * (g->d - t));

  calc_rpcVraps(g, maxCoeff, t, ret_coeffs);

  for(wire_t i = 0; i <= g->numTotProbes; i++)
    ret_coeffs[i] *= sdMax;
}
