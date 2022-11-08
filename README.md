# Random Probing Security Checker

This tool checks the random probing security properties of a given gadget.

## Execution

To compile

    ./compile.sh

To get the coefficient for Random Probing Security:

    ./rpsc --sage 'file gadget .sage' -c 'max coefficient' 'op'

Where op can be either '--rpsCor1', '--rpsCor2', '--rpsCor3'.

To get the coefficient for Random Probing Composability:

    ./rpsc --sage 'file gadget .sage' -c 'max coefficient' 'op' -t 'max number of safely leaking shares'

Where op can be either '--rpcCor1', '--rpcCor2'.

To get informations on a gadget and an internal representation that can be used to check if it was read correctly do:

    ./rpsc --sage 'file gadget .sage' --printGadget

For the .py that generate .sage, it's possible to do (at least in bash):

    ./rpsc --sage <('generator.py' 'parameters') -c 'max coefficient' 'op'

The '--help' prints this readme and terminates, the '--license' prints the GPLv3 and terminates.


TODO: is the -c mandatory?

Note: In the .sage files:
 - all assignments must have exactly one operation (+ or *)
 - no input must be used directly as output  TODO: is it still valid?
 - no input must be written to


## Graphs

the paper's graphs were made with (and by fiddling the internal parameters):

    ./plot_test_fn.py vraps/otpoePaper_add.py__3/rpsVraps/*.success

    ./plot_time_acc.py rps isw_mul.py__3:{3:green,5:blue,6:orange,8:black}
    ./plot_time_acc.py rps vrapsPaper_mul.sage:{3:green,4:blue,5:orange,6:black}

    ./plot_time_acc.py rps vrapsPaper_add_v3.sage:{3:green,5:blue,7:orange,9:black,12:fuchsia,14:purple}
    ./plot_time_acc.py rps otpoePaper_small_add.sage:{3:green,5:blue,7:orange,9:black}
    ./plot_time_acc.py rps otpoePaper_add.py__3:{3:green,5:blue,7:orange,9:black}

    ./plot_time_acc.py rps vrapsPaper_copy.sage:{3:green,5:blue,7:orange,12:fuchsia,13:black}
    ./plot_test_fn.py {test,vraps}/vrapsPaper_copy.sage/rps*/13.success

The 'plot_time_acc.py' script will create the file 'toDelete_out.png'

TODO: what about the vraps's result?

## Tests

TODO

## Copyright

Copyright (C) 2022  Giuseppe Manzoni.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

