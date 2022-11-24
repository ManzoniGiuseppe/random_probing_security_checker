# Random Probing Security Checker

This tool checks the random probing security properties of a given gadget.

## Compilation

To compile for serial execution

    ./compile.sh

To compile so that it execs on N threads (note that -t 0 is to disable multithreading, -t 1 uses the multithreading library but only 1 thread):

    ./compile.sh -t N

In both cases the compilation produces an executable called 'rpsc'

## Execution

Exectly an input parameter must be specified, and this tool currently supports the following input formats:
 - '--sage <file>' for sage files, also supported by VRAPS and IronMask.

This tool supports pipes, so in bash it's possible to use something like '--sage <(isw_mul.py 5)' in case one has a script 'isw_mul.py' that generates that gadget on the fly given the number of shares.

It supports the following RPS operations:
 - '--rpsCor1' for the most accurate and slower
 - '--rpsCor2' less accuracy and faster
 - '--rpsCor3' even lower accuracy and faster
 - '--rpsVraps' the one calculated by VRAPS (which is not complete and may return slightly higher coefficents). Same execution time of '--rpsCor3' but less accurate

It supports the following RPC operations:
 - '--rpcCor1' for the most accurate and slower
 - '--rpcCor2' for less accuracy and faster
 - '--rpcVraps' for the one calculated by VRAPS. Same execution time of '--rpcCor2' but less accurate.

In case an RPC operation is used, exactly one of the following options are mandatory:
 - '-t <max safe number of shares>' see the paper. It must be 0 <= t < d.

It's also possible to specify:
 - '--help' to show this README.md
 - '--printGadget' to obtain information on the gadget and to check it's read correctly by this tool.
 - '-c <max coeff>' to limit the number of coefficients to calculate, the other coefficents will be set to their maximum.

## Inputs

Note: In the .sage files:
 - names are alphanumeric
 - the # directives have no indentation and must all be at the start
 - any other # is a line-long comment
 - only binary + and *

## Other

See [this readme](README_other_tools.md) for how to use the other tools provided with this tool.

## Copyright

Copyright (C) 2022  Giuseppe Manzoni.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

