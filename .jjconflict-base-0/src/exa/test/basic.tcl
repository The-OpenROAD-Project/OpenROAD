# Example test
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def basic.def

# Not needed for the test - just here for demonstration purposes
exa::set_debug

example_instance -name test

set def_file [make_result_file basic.def]
write_def $def_file
diff_file basic.defok $def_file
