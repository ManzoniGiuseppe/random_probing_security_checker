#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

outDir="../.rpsc.out"

if [ -e .regression.test ] ; then
  echo "Please delete '.regression.test' to confirm" 2>&1
  exit 1
fi

for f in $(cd $outDir ; ls */*/*.success) ; do
  echo -n "${f}: "
  head -n 1 ${outDir}/${f}
done | sort > .regression.test
