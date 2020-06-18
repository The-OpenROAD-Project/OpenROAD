# gcd full meal deal
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal3
# flute results are not stable on nets with fanout > 9
set_load 20 _241_

report_worst_slack

set buffer_cell BUF_X4
set_dont_use {CLKBUF_* AOI211_X1 OAI211_X1}

resize
buffer_ports -buffer_cell $buffer_cell
repair_max_cap -buffer_cell $buffer_cell
repair_max_slew -buffer_cell $buffer_cell
set_max_fanout 100 [current_design]
repair_max_fanout -buffer_cell $buffer_cell
repair_long_wires -max_length 80 -buffer_cell $buffer_cell
resize

repair_tie_fanout LOGIC0_X1/Z
repair_tie_fanout LOGIC1_X1/Z
repair_hold_violations -buffer_cell $buffer_cell

report_checks
report_check_types -max_slew -max_fanout -max_capacitance
report_worst_slack
report_long_wires 10

report_floating_nets -verbose
report_design_area
