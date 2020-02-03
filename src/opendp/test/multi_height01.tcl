source helpers.tcl
read_lef -tech iccad17-bench/des_perf_a_md1/tech.lef
read_lef -lib iccad17-bench/des_perf_a_md1/cells_modified.lef
read_def multi_height01.def
legalize_placement
set def_file [make_result_file multi_height01.def]
write_def $def_file
diff_file $def_file multi_height01.defok
