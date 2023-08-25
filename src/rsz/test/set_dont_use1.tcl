# set_dont_use on cell that does not exists

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"
set_dont_use "non_existent_cell"
