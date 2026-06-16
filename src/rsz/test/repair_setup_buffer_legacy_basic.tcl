# Legacy BufferMove parity test on a setup-critical fanout path.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_worst_slack -max
report_tns -digits 3
repair_timing -setup -sequence "buffer" -skip_last_gasp
report_worst_slack -max
report_tns -digits 3
report_checks -fields input -digits 3
