# Exercise PARALLEL_EVAL target-parallel scoring and conflict-filtered batch commit.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_path11.def
read_sdc repair_setup_path11.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -phases "PARALLEL_EVAL" -max_iterations 2 -verbose
