# gcd flow pipe cleaner
source "helpers.tcl"
source "Nangate45/Nangate45.vars"

set synth_verilog "gcd_nangate45.v"
set design "gcd"
set top_module "gcd"
set sdc_file "gcd_nangate45.sdc"
set init_floorplan_cmd "initialize_floorplan -site $site \
    -die_area {0 0 100.13 100.8} \
    -core_area {10.07 11.2 90.25 91} \
    -tracks $tracks_file"
set max_drv_count 4

source -echo "flow.tcl"
