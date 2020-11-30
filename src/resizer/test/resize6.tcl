# resize with buffer outputs with external load
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg3.def
create_clock -period 1 clk

buffer_ports -outputs
# 2pf
set_load -pin_load 2000 out
repair_design

report_net -connections -verbose out
report_check_types -max_slew -max_capacitance
