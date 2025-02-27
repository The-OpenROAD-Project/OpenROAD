# Test effectiveness of IO PAD -> Macro connection.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef
read_lef testcases/macro_only.lef

# libs are needed, because the IO PADs are created based on the
# internal MPL dataflow data structure and, while creating this
# structure, if no std cell has liberty data, the dataflow creation
# is skipped.
read_liberty Nangate45/Nangate45_fast.lib
read_liberty Nangate45_io/dummy_pads.lib

read_def testcases/io_pads1.def

set_thread_count 0
rtl_macro_placer -report_directory results/io_pads1 -halo_width 4.0

set def_file [make_result_file "io_pads1.def"]
write_def $def_file
diff_files $def_file "io_pads1.defok"
