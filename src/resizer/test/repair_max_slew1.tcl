# repair_max_slew reg3
source "helpers.tcl"
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg3.def
create_clock clk -period 1
set_input_delay -clock clk 0 in1
# driving cell so input net has non-zero slew
set_driving_cell -lib_cell snl_bufx1 [get_ports in1]
# sdc slew constraint applies to input ports
set_max_transition 1.0 [get_ports *]

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
# kohm/micron, pf/micron
# use 10x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 1.3e-3

report_check_types -max_transition -all_violators

repair_max_slew -buffer_cell $buffer_cell

report_check_types -max_transition -all_violators
report_checks -fields {input_pin transition_time capacitance}

set def_file [make_result_file repair_max_slew1.def]
write_def $def_file
diff_files $def_file repair_max_slew1.defok
