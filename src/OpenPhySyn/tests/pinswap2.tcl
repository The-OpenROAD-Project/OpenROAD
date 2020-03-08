source helpers.tcl
psn::set_log_pattern "\[%^%l%$\] %v"
puts [psn::has_transform pin_swap]
read_liberty ../tests/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib
read_lef -tech -library ../tests/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
read_def pinswap1.def

set_units -time ns
create_clock -name clk -period 10

# report_power
optimize_power
# report_power
set def_file [make_result_file pinswap2.def]
write_def $def_file
diff_file $def_file pinswap2.defok
