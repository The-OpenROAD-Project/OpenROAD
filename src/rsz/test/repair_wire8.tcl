# repair_wire1 with fast/slow corners
# in1--u1--u2--------u3-out1
#             2000u
source "helpers.tcl"
define_corners slow fast
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
# zero estimated parasitics to output port
set_load 0 [get_net out1]

repair_design

report_check_types -max_slew -max_capacitance
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
