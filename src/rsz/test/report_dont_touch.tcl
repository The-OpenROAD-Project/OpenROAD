# check for report_dont_touch

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"

report_dont_touch

set_dont_touch "_505_"
set_dont_touch "_05*_"

report_dont_touch
