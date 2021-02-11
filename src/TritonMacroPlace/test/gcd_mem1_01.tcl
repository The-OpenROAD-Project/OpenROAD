# gcd 1 macro halo=.5
source helpers.tcl
set design gcd_mem1 
set tech_dir nangate45-bench/tech
set test_name "gcd_mem1_01"

read_liberty ${tech_dir}/NangateOpenCellLibrary_typical.lib
read_liberty ${tech_dir}/fakeram45_64x7.lib

read_lef ${tech_dir}/NangateOpenCellLibrary.lef
read_lef ${tech_dir}/fakeram45_64x7.lef

read_def gcd_mem1.def
read_sdc gcd.sdc

macro_placement -global_config halo_0.5.cfg

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
