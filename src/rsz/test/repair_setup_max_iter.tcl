# Regression: repair_timing -setup must not crash when -max_passes and
# -max_iterations are small enough to exit the inner loop on a pass where the
# `better` branch would otherwise leave the journal double-closed.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -max_passes 1 -max_iterations 1
