source helpers.tcl
set test_name simple01-td
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./$test_name.def

create_clock -name core_clock -period 2 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock  -layer metal5

#global_placement -skip_initial_place -disable_routability_driven
#global_placement -disable_timing_driven -disable_routability_driven
#global_placement -disable_timing_driven
#global_placement
global_placement -disable_routability_driven

# check reported wns
estimate_parasitics -placement
report_worst_slack

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
