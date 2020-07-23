# gcd flow pipe cleaner
source "helpers.tcl"
source "sky130/sky130.vars"

set synth_verilog "gcd_sky130.v"
set design "gcd"
set top_module "gcd"
set sdc_file "gcd_sky130.sdc"
set die_area {0 0 299.96 300.128}
set core_area {9.996 10.08 289.964 290.048}
# replace barfs with default density penalty 8e-5
set global_place_density_penalty 4e-5
set max_drv_count 1

source -echo "flow.tcl"
