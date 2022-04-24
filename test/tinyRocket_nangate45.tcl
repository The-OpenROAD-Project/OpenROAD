# tinyRocket flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "Nangate45/Nangate45.vars"

set design "tinyRocket"
set top_module "RocketTile"
set synth_verilog "tinyRocket_nangate45.v"
set sdc_file "tinyRocket_nangate45.sdc"
set rams {fakeram45_64x32}
set extra_lef {}
set extra_liberty {}
foreach ram $rams {
  lappend extra_lef "Nangate45/$ram.lef"
  lappend extra_liberty "Nangate45/$ram.lib"
}
set die_area {0 0 924.92 799.4}
set core_area {10.07 9.8 914.85 789.6}

set cap_margin 35

source -echo "flow.tcl"
