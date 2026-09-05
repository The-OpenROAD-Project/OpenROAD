# Compare full timing-update runtime on the placed Nangate45 Ibex design.
set test_name dcalc_full_update_runtime_nangate45_ibex
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def ../../gpl/test/design/nangate45/ibex_core/ibex_core.def
read_sdc ../../gpl/test/design/nangate45/ibex_core/ibex_core.sdc
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

run_dcalc_full_update_runtime
