# Regression for journal stack invariant at pass==max and LastGasp
# num_viols double-decrement after accept-crosses-margin.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -max_passes 1 -max_iterations 1
repair_timing -setup -phases LAST_GASP -setup_margin -0.15 -verbose
