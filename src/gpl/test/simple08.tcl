source helpers.tcl
set test_name simple08 
read_lef ./sky130hd.lef
read_def ./$test_name.def

global_placement -density 0.75 -bin 64 -overflow 0.2
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
