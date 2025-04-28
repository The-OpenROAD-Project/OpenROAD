# Test if the bundled nets inside annealing are correct for a block with
# two blocked regions for pins and Macro -> IO connections.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_liberty Nangate45/Nangate45_fast.lib

read_def testcases/io_constraints6.def

exclude_io_pin_region -region bottom:* -region left:* \
                      -region top:40-150 -region right:40-125

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints6 -halo_width 4.0

set def_file [make_result_file "io_constraints6.def"]
write_def $def_file
diff_files $def_file "io_constraints6.defok"

