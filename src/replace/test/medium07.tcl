source helpers.tcl
set test_name medium07
read_lef ./nangate45.lef
read_lef ./bp_fe_top_macro.lef 
read_def ./$test_name.def

global_placement 
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
source report_hpwl.tcl
exit
