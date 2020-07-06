# aes flow pipe cleaner
source "helpers.tcl"
source "Nangate45/Nangate45.vars"

set synth_verilog "aes_nangate45.v"
set design "aes"
set top_module "aes_cipher_top"
set sdc_file "aes_nangate45.sdc"
# These values must be multiples of placement site
# x=0.19 y=1.4
set init_floorplan_cmd "initialize_floorplan -site $site \
    -die_area {0 0 620.15 620.6} \
    -core_area {10.07 11.2 610.27 610.8} \
    -tracks $tracks_file"
set max_drv_count 30

source -echo "flow.tcl"
