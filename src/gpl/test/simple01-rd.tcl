source helpers.tcl
set test_name simple01-rd
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./$test_name.def

# Testing for routability with high target RC value,routability should not be activated.
# Using GRT for congestion estimantion.
global_placement -routability_driven -routability_use_grt -routability_target_rc_metric 1.25

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
