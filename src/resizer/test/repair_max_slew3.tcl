# repair_max_slew reg3
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg3.def
create_clock clk -period 1
set_input_delay -clock clk 0 in1
# driving cell so input net has non-zero slew
set_driving_cell -lib_cell BUF_X1 [get_ports in1]
# sdc slew constraint applies to input ports
set_max_transition 1.0 [get_ports *]

# kohm/micron, pf/micron
# use 10x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 1.3

report_check_types -max_slew -violators

repair_max_slew -buffer_cell BUF_X2

report_check_types -max_slew -violators
report_checks -fields {input_pin transition_time capacitance}

set def_file [make_result_file repair_max_slew1.def]
write_def $def_file
diff_files repair_max_slew1.defok $def_file
