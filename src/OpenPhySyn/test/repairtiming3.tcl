source helpers.tcl
psn::set_log_pattern "\[%^%l%$\] %v"
puts [psn::has_transform repair_timing]
read_lef ../test/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
read_def repairtiming1.def
read_liberty ../test/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
create_clock -name core_clock -period 4.1 [get_ports {clk_i}]
set_wire_rc -resistance 1.0e-03 -capacitance 1.0e-03
report_wns -digit 3
report_tns -digit 3
repair_timing -negative_slack_violations -iterations 1 -auto_buffer_library small
report_wns -digit 3
report_tns -digit 3
set def_file [make_result_file repairtiming3.def]
write_def $def_file
diff_file $def_file repairtiming3.defok