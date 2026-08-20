# Compare full timing-update runtime on the placed Nangate45 GCD design.
set test_name dcalc_full_update_runtime_gcd
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def ../../rsz/test/gcd_nangate45_placed.def
read_sdc ../../rsz/test/gcd_nangate45.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

run_dcalc_full_update_runtime
