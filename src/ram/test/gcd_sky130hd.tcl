# gcd flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

set synth_verilog "gcd_sky130hd.v"
set design "gcd"
set top_module "gcd"
set sdc_file "gcd_sky130hd.sdc"
set die_area {0 0 299.96 300.128}
set core_area {9.996 10.08 289.964 290.048}

source -echo "flow.tcl"
