# Critical-path slice extracted from nangate45.  Drives LEGACY repair_setup.

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_path11.def
read_sdc repair_setup_path11.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
