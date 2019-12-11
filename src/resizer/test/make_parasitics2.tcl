# set_wire_rc -layer
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

set_wire_rc -layer M2
report_checks
