# jpeg sky130hs 96017 insts
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

set design "jpeg"
set top_module "jpeg_encoder"
set synth_verilog "jpeg_sky130hd.v"
set sdc_file "jpeg_sky130hd.sdc"
# These values must be multiples of placement site
set die_area {0 0 3000.04 2999.8}
set core_area {10.07 9.8 2989.97 2990}

set max_slew_margin 20
set max_cap_margin 20

source -echo "flow.tcl"
