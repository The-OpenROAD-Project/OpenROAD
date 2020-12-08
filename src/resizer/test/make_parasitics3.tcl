# estimate_parasitics propagated clock
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg6.def

create_clock -period 10 clk
set_propagated_clock clk
set_input_delay -clock clk 0 in1

set_wire_rc -layer metal2
estimate_parasitics -placement

report_checks -format full_clock
