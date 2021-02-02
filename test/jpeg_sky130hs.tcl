# jpeg sky130
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hs/sky130hs.vars"

set design "jpeg"
set top_module "jpeg_encoder"
set synth_verilog "jpeg_sky130hs.v"
set sdc_file "jpeg_sky130hs.sdc"
# These values must be multiples of placement site
set die_area {0 0 3000.04 2999.8}
set core_area {10.07 9.8 2989.97 2990}
set max_drv_count 1
# liberty units (ns)
set setup_slack_limit -4.5
set hold_slack_limit 0.0

source -echo "flow.tcl"
