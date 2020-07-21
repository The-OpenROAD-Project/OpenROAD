# aes flow pipe cleaner
source "helpers.tcl"
source "sky130/sky130.vars"

set design "aes"
set top_module "aes_cipher_top"
set synth_verilog "aes_sky130.v"
set sdc_file "aes_sky130.sdc"
set die_area {0 0 620.15 620.6}
set core_area {10.07 11.2 610.27 610.8}
set max_drv_count 1

source -echo "flow.tcl"
