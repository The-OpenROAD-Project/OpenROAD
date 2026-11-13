# LEGACY repair_setup on a critical-path slice of sky130hd aes_cipher_top with
# 13 violating endpoints. The golden log covers buffer removal, sizing up, pin
# swaps and buffering, and its worst path runs from a flip-flop to a flip-flop.

source "helpers.tcl"

read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup_path14.def
read_sdc repair_setup_path14.sdc

source sky130hd/sky130hd.rc
set_wire_rc -clock -layer met5
set_wire_rc -signal -layer met2
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
