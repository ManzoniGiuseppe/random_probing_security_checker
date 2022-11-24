# Random Probing Security Checker - Other Tools

TODO

## Graphs

the thesis' graphs were made with (and by fiddling the internal parameters to improve the layout):

    ./plot_test_fn.py .vraps.out/otpoePaper_add.py__3/rpsVraps/*.success


    ./plot_time_acc.py rps isw_mul.py__3:{3:green,5:blue,6:orange,8:black}
    ./plot_time_acc.py rps vrapsPaper_mul.sage:{3:green,4:blue,5:orange,6:black}
    ./plot_time_acc.py rps otpoePaper_mul.py__3:{3:green,4:blue,5:orange}

    ./plot_time_acc.py rps vrapsPaper_add_v3.sage:{3:green,5:blue,7:orange,9:black,12:fuchsia,14:purple}
    ./plot_time_acc.py rps otpoePaper_small_add.sage:{3:green,5:blue,7:orange,9:black}
    ./plot_time_acc.py rps otpoePaper_add.py__5:{3:green,5:blue,6:orange,7:black}

    ./plot_time_acc.py rps vrapsPaper_copy.sage:{3:green,5:blue,7:orange,12:fuchsia,13:black}
    ./plot_test_fn.py .{rpsc,vraps}.out/vrapsPaper_copy.sage/rps*/13.success


    ./plot_time_acc.py rpc vrapsPaper_mul.sage:{2:green,3:blue,4:orange,5:black}
    ./plot_time_acc.py rpc otpoePaper_small_mul.sage:{2:green,3:blue,4:orange,5:black}

    ./plot_time_acc.py rpc vrapsPaper_add_v3.sage:{2:green,3:blue,4:orange,5:black}
    ./plot_time_acc.py rpc otpoePaper_small_add.sage:{2:green,3:blue,4:orange,5:black}

    ./plot_time_acc.py rpc otpoePaper_copy.py__3:{3:green,4:blue,5:orange,6:black}

    ./plot_time_acc.py rpc otpoePaper_small_refresh.sage:{3:green,4:blue,5:orange,6:black}
    ./plot_time_acc.py rpc isw_refresh.py__3:{3:green,5:blue,7:orange,9:black}


    ./plot_time_acc.py rps isw_mul.py__3:8:blue
    ./plot_time_acc.py rpc isw_mul.py__3:5:blue


This needs a directory '.vraps.out' that should be created by a script similar to 'generate_out.sh' only that parses VRAPS' output.

The 'plot_time_acc.py' script will create the file '.plot_time_acc.out.png'

## Tests

TODO describe tests and other sub-programs

## Copyright

Copyright (C) 2022  Giuseppe Manzoni.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

