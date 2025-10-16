source helpers.tcl
set test_name gcd_fill

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def gcd_prefill.def
density_fill -rules fill.json

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
