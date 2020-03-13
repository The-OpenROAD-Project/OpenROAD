source helpers.tcl
psn::set_log_pattern "\[%^%l%$\] %v"
puts [psn::has_transform pin_swap]
read_liberty ../test/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
read_lef ../test/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
read_def pinswap1.def

set_units -time ns
create_clock [get_ports clk]  -name core_clock  -period 10

set_wire_rc -layer metal2

report_checks -digit 5
optimize_design -no_gate_clone
report_checks -digit 5
set def_file [make_result_file pinswap1.def]
write_def $def_file
diff_file $def_file pinswap1.defok
