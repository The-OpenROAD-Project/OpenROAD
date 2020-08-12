source helpers.tcl
set design gcd_mem3 
set tech_dir nangate45-bench/tech
set design_dir nangate45-bench/design/${design}
set test_name "gcd-mem3-test-01"

read_liberty ${tech_dir}/NangateOpenCellLibrary_typical.lib
read_liberty ${tech_dir}/fakeram45_64x7.lib

read_lef ${tech_dir}/NangateOpenCellLibrary.lef
read_lef ${tech_dir}/fakeram45_64x7.lef

read_def ${design_dir}/${design}.def
read_sdc ${design_dir}/${design}.sdc

macro_placement -global_config ${design_dir}/halo_0.5.cfg

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
