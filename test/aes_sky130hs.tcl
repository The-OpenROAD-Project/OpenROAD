# aes flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hs/sky130hs.vars"

set design "aes"
set top_module "aes_cipher_top"
set synth_verilog "aes_sky130hs.v"
set sdc_file "aes_sky130hs.sdc"
set die_area {0 0 2000 2000}
set core_area {30 30 1770 1770}

# Note many slew/cap violations are in the CLOCK NETWORK
# where margins do not help
set slew_margin 15
set cap_margin 15

source -echo "flow.tcl"
