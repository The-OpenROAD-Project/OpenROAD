# set_wire_rc -layer
read_lef Nangate.lef
read_liberty Nangate_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

set_wire_rc -layer metal2
report_checks
