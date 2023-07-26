source helpers.tcl

read_lef  Nangate45.lef
read_def gcd.def
read_liberty NangateOpenCellLibrary_typical.lib
read_sdc gcd.sdc
analyze_power_grid -net VSS
