# gcd flow pipe cleaner
source "helpers.tcl"
source "flow_helpers.tcl"
source "asap7/asap7.vars"

set design "aes"
set top_module "aes_cipher_top"
set synth_verilog "aes_asap7.v"
set sdc_file "aes_asap7.sdc"
set die_area {0.0 0.0 68.746 68.746}
set core_area {2.052 2.160 66.744 66.690}

include -echo "flow.tcl"
