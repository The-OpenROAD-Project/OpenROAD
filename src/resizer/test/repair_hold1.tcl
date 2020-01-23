source helpers.tcl
read_liberty nlc18.lib
read_lef nlc18.lef
read_verilog repair_hold1.v
link_design top

create_clock -period 2 clk
set_input_delay -clock clk 0 {in1 in2}
set_propagated_clock clk

set buffer_cell [get_lib_cell nlc18/snl_bufx2]
set_wire_rc -layer metal1

report_checks -path_delay min -to r3/D
sta::resizer_preamble [get_libs "nlc18"]
sta::repair_pin_hold_violations [get_pins r3/D] [get_lib_cell nlc18/snl_bufx2]
report_checks -path_delay min -to r3/D

set def_file [make_result_file repair_hold1.def]
write_def $def_file
diff_files $def_file repair_hold1.defok
