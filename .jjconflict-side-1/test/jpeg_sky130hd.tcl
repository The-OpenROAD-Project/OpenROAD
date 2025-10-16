# jpeg sky130hs 96017 insts
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

set design "jpeg"
set top_module "jpeg_encoder"
set synth_verilog "jpeg_sky130hd.v"
set sdc_file "jpeg_sky130hd.sdc"
# These values must be multiples of placement site
set die_area {0 0 1500 1500}
set core_area {10 10 1490 1490}

set slew_margin 20
set cap_margin 20

set global_place_density 0.5

include -echo "flow.tcl"
