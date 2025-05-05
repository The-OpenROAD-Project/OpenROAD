# Test if the available regions created based on the blocked regions for pins
# are correctly generated when there are no constraints at all. Connections
# are needed so we trigger the closest available region distance computation
# inside the annealer.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_liberty Nangate45/Nangate45_fast.lib

read_def testcases/io_constraints6.def

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints8 -halo_width 4.0

set def_file [make_result_file "io_constraints8.def"]
write_def $def_file
diff_files $def_file "io_constraints8.defok"

