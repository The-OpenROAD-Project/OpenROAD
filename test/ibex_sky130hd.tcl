# ibex sky130
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

set design "ibex"
set top_module "ibex_core"
set synth_verilog "ibex_sky130hd.v"
set sdc_file "ibex_sky130hd.sdc"
set die_area {0 0 3000.08 2999.8}
set core_area {10.07 11.2 2990.01 2990}

source -echo "flow.tcl"
