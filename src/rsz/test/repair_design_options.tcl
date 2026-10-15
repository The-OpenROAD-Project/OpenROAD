# The repair_design switches generated from RepairDesignOptions give the
# same repair as repair_wire2.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire2.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# zero estimated parasitics to output ports
set_load 0 [get_net out1]
set_load 0 [get_net out2]

report_options repair_design -max_wire_length 1000 -slew_margin 5
repair_design -max_wire_length 1000

report_long_wires 6
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out2
