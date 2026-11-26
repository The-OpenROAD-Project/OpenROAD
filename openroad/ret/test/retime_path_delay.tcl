# A path delay between two named registers is an explicit requirement on
# that path: the registers stay, or the requirement would go with the one
# that moved.  A path delay on every register ([all_registers]) names a
# class, not cells, and does not keep them in place.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.13 clk

set_max_delay 0.05 -from [get_cells r1] -to [get_cells r2]
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
unset_path_exceptions -from [get_cells r1] -to [get_cells r2]

set_max_delay 0.2 -from [all_inputs] -to [all_registers]
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
