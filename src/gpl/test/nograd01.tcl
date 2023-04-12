source helpers.tcl
set test_name nograd01

read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_def $test_name.def

global_placement -skip_io

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
