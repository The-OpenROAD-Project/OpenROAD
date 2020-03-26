# repair_max_slew reg3 -max_utilization (no core size)
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg5.def
create_clock clk -period 1
set_input_delay -clock clk 0 in1
# driving cell so input net has non-zero slew
set_driving_cell -lib_cell snl_bufx1 [get_ports in1]
# sdc slew constraint applies to input ports
set_max_transition 1.0 [get_ports *]

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
# kohm/micron, pf/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 1.3e-2
report_design_area

repair_max_slew -buffer_cell $buffer_cell -max_utilization 70
report_design_area
