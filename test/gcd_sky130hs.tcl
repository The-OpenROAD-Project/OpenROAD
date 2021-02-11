# gcd flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hs/sky130hs.vars"

set synth_verilog "gcd_sky130hs.v"
set design "gcd"
set top_module "gcd"
set sdc_file "gcd_sky130hs.sdc"
set die_area {0 0 299.96 300.128}
set core_area {9.996 10.08 289.964 290.048}

set max_drv_count 1
# liberty units (ns)
set setup_slack_limit -0.04
set hold_slack_limit 0.0

source -echo "flow.tcl"
