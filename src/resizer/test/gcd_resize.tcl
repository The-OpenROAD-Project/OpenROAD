# gcd full meal deal
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set buffer_cell BUF_X4
set_wire_rc -layer metal2
buffer_ports -buffer_cell $buffer_cell
resize -dont_use CLKBUF_*
repair_max_cap -buffer_cell $buffer_cell
repair_max_slew -buffer_cell $buffer_cell
repair_max_fanout -max_fanout 100 -buffer_cell $buffer_cell
repair_hold_violations -buffer_cell $buffer_cell
report_floating_nets -verbose
report_design_area
