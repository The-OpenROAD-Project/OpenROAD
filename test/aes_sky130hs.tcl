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
set max_drv_count 1
# liberty units (ns)
set setup_slack_limit -0.5
set hold_slack_limit 0.0

source -echo "flow.tcl"
