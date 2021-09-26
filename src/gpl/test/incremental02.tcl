# This is a version of aes where some of the gates have been unplaced to
# simulate rmp.

source helpers.tcl
set test_name incremental02
read_lef ./nangate45.lef
read_def ./$test_name.def

global_placement -incremental -density 0.3 -pad_left 2 -pad_right 2
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
