# Critical-path slice extracted from sky130hd.  Drives LEGACY repair_setup.

source "helpers.tcl"

read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup_path15.def
read_sdc repair_setup_path15.sdc

source sky130hd/sky130hd.rc
set_wire_rc -clock -layer met5
set_wire_rc -signal -layer met2
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
