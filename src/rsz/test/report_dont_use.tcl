# check for report_dont_use

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"

report_dont_use

set_dont_use "CLKBUF*"
set_dont_use "INV*"

report_dont_use
