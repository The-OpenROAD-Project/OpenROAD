# check for library set dont use

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"

report_dont_use

# unset dont_use from library
unset_dont_use "ANTENNA_X1"

report_dont_use

reset_dont_use

report_dont_use
