# check for report_dont_use with corners

source "helpers.tcl"

read_liberty Nangate45/Nangate45_slow.lib
read_liberty Nangate45/Nangate45_fast.lib
define_scene slow -liberty Nangate45/Nangate45_slow.lib
define_scene fast -liberty Nangate45/Nangate45_fast.lib
set_scene slow
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"

report_dont_use

set_dont_use "CLKBUF*"
set_dont_use "INV*"

report_dont_use
