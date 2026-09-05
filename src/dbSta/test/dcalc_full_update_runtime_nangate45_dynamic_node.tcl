# Compare full timing-update runtime on the placed Nangate45 dynamic-node design.
set test_name dcalc_full_update_runtime_nangate45_dynamic_node
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def ../../gpl/test/design/nangate45/dynamic_node_top_wrap/dynamic_node_top_wrap.def
read_sdc ../../gpl/test/design/nangate45/dynamic_node_top_wrap/dynamic_node_top_wrap.sdc
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

run_dcalc_full_update_runtime
