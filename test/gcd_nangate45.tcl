# gcd flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "Nangate45/Nangate45.vars"

set design "gcd"
set top_module "gcd"
set synth_verilog "gcd_nangate45.v"
set sdc_file "gcd_nangate45.sdc"
set die_area {0 0 100.13 100.8}
set core_area {10.07 11.2 90.25 91}

source -echo "flow.tcl"
