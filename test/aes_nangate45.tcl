# aes flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "Nangate45/Nangate45.vars"

set design "aes"
set top_module "aes_cipher_top"
set synth_verilog "aes_nangate45.v"
set sdc_file "aes_nangate45.sdc"
set die_area {0 0 1020 920.8}
set core_area {10 12 1010 911.2}

set max_cap_margin 10

source -echo "flow.tcl"
