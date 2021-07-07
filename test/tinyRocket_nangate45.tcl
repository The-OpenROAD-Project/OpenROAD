# tinyRocket flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "Nangate45/Nangate45.vars"

set design "tinyRocket"
set top_module "RocketTile"
set synth_verilog "tinyRocket_nangate45.v"
set sdc_file "tinyRocket_nangate45.sdc"
set extra_lef [glob "Nangate45/fakeram45*.lef"]
set extra_liberty [glob "Nangate45/fakeram45*.lib"]
set die_area {0 0 924.92 799.4}
set core_area {10.07 9.8 914.85 789.6}

set max_cap_margin 20

source -echo "flow.tcl"
