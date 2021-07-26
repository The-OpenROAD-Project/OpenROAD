# ibex sky130hs
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hs/sky130hs.vars"

set design "ibex"
set top_module "ibex_core"
set synth_verilog "ibex_sky130hs.v"
set sdc_file "ibex_sky130hs.sdc"
set die_area {0 0 3000.08 2999.8}
set core_area {10.07 11.2 2990.01 2990}

set max_slew_margin 25
set max_cap_margin 25

source -echo "flow.tcl"
