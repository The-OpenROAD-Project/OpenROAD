# gcd flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "asap7/asap7.vars"

set design "gcd"
set top_module "gcd"
set synth_verilog "gcd_asap7.v"
set sdc_file "gcd_asap7.sdc"
set die_area {0 0 16.2 16.2}
set core_area {1.08 1.08 15.12 15.12}

include -echo "flow.tcl"
