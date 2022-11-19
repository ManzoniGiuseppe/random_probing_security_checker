#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

outParallelDir=".rpsc.out-"
maxParallel=4 #$(ls -d ${outParallelDir}* | sed "s/${outParallelDir}//" | sort | tail -n 1)

function timeToInt(){
  python3 -c "print($(echo $1 | sed 's/s//;s/:/ * 60 + /'))"
}

if [ -e .regression.test ] ; then
  echo "Please delete '.regression.test' to confirm" 2>&1
  exit 1
fi

for t in $(seq 2 $maxParallel) ; do
  for f in $(cd "${outParallelDir}${t}" ; ls -d */* | while read d ; do ls $d/*.success | tail -n 1 ; done) ; do
    if [ -f "${outParallelDir}1/${f}" ] ; then
      serialReal=$(timeToInt $(head -n 1 "${outParallelDir}1/${f}"))
      serialUser=$(timeToInt $(head -n 2 "${outParallelDir}1/${f}" | tail -n 1))
      serialSys=$(timeToInt $(tail -n 1 "${outParallelDir}1/${f}"))
      serialExecution=$(python3 -c "print($serialUser + $serialSys)")

      parallelReal=$(timeToInt $(head -n 1 "${outParallelDir}${t}/${f}"))
      parallelUser=$(timeToInt $(head -n 2 "${outParallelDir}${t}/${f}" | tail -n 1))
      parallelSys=$(timeToInt $(tail -n 1 "${outParallelDir}${t}/${f}"))
      parallelExecution=$(python3 -c "print($parallelUser + $parallelSys)")

      if [ $(echo "$serialExecution" | sed "s/\..*//") -ge 1 ] ; then # if more than 1s
        parallelTime=$(python3 -c "print($serialExecution - $t/($t-1)*($parallelReal - $parallelExecution/$t))")
        p=$(python3 -c "print($parallelTime / $serialExecution)")
        serialTime=$(python3 -c "print($serialExecution - $parallelTime)")
        sp=$(python3 -c "print( $parallelTime / ($parallelReal - $serialTime) )")
        s=$(python3 -c "print($serialReal / $parallelReal)")

        echo "${outParallelDir}${t}/${f}:${sp}:${p}:${s}"
      fi
    fi
  done
done | sort > .rpsc.out.stat
