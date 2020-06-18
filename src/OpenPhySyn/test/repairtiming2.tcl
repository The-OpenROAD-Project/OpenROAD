source helpers.tcl
psn::set_log_pattern "\[%^%l%$\] %v"
puts [psn::has_transform repair_timing]
read_lef ../test/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
read_def repairtiming1.def
read_liberty ../test/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
create_clock -name core_clock -period 10.0 [get_ports {clk_i}]
set_wire_rc -resistance 2.0e-03 -capacitance 2.0e-03
report_check_types -max_capacitance -max_slew -violators
repair_timing -transition_violations -capacitance_violations -iterations 1 -auto_buffer_library small -pin_swap_disabled
report_check_types -max_capacitance -max_slew -violators
set def_file [make_result_file repairtiming2.def]
write_def $def_file
diff_file $def_file repairtiming2.defok