# check for report_dont_use with corners

source "helpers.tcl"

define_corners slow fast
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"

report_dont_use

set_dont_use "CLKBUF*"
set_dont_use "INV*"

report_dont_use
