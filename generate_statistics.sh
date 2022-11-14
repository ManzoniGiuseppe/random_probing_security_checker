#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

outSerialDir=".rpsc.out"
outParallelDir=".rpsc.out-"

function timeToInt(){
  python3 -c "print($(echo $1 | sed 's/s//;s/:/ * 60 + /'))"
}

if [ -e .regression.test ] ; then
  echo "Please delete '.regression.test' to confirm" 2>&1
  exit 1
fi

for t in $(seq 2 4) ; do
  for f in $(cd "${outParallelDir}${t}" ; ls -d */* | while read d ; do ls $d/*.success | tail -n 1 ; done) ; do
    if [ -f "${outSerialDir}/${f}" ] ; then
      serialExecution=$(timeToInt $(tail -n 1 "${outSerialDir}/${f}"))
      real=$(timeToInt $(head -n 1 "${outParallelDir}${t}/${f}"))
      user=$(timeToInt $(head -n 2 "${outParallelDir}${t}/${f}" | tail -n 1))
      sys=$(timeToInt $(tail -n 1 "${outParallelDir}${t}/${f}"))

      if [ $(echo "$serialExecution" | sed "s/\..*//") -ge 1 ] ; then
        timeParallelSync=$(python3 -c "print($user + $sys - $serialExecution)")
        timeParallelizable=$(python3 -c "print(($user + $sys - $real)*${t}/(${t}-1) - $timeParallelSync)")
        timeSerial=$(python3 -c "print($serialExecution - $timeParallelizable)")

        inefficency=$(python3 -c "print($timeParallelSync / ($timeParallelizable + $timeParallelSync) * 100)")
        leftToSerialize=$(python3 -c "print($timeSerial / $serialExecution * 100)")

        echo "${outParallelDir}${t}/${f}:${inefficency}:${leftToSerialize}"
      fi
    fi
  done
done | sort > .rpsc.out.stat
