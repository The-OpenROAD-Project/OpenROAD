# Legacy UnbufferMove removes this buffer chain after estimatedSlackOK;
# the new optimizer must not add a second stage-delay improvement gate.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_unbuffer_legacy_bufx2.def
create_clock -period 0.05 clk
set_load 0.0 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
repair_timing -setup -sequence "unbuffer" -max_passes 20
report_checks -fields input -digits 3
