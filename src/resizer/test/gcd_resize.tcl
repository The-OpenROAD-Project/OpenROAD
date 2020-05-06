# gcd full meal deal
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal2

report_worst_slack

set buffer_cell BUF_X4
buffer_ports -buffer_cell $buffer_cell
repair_max_cap -buffer_cell $buffer_cell
repair_max_slew -buffer_cell $buffer_cell
repair_max_fanout -max_fanout 100 -buffer_cell $buffer_cell
resize -dont_use {CLKBUF_* AOI211_X1 OAI211_X1}
repair_tie_fanout -max_fanout 100 Nangate_typ/LOGIC0_X1/Z
repair_tie_fanout -max_fanout 100 Nangate_typ/LOGIC1_X1/Z
repair_hold_violations -buffer_cell $buffer_cell

report_floating_nets -verbose
report_design_area
report_worst_slack

