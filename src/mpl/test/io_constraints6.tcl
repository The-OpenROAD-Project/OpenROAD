# Test if the bundled nets inside annealing are correct for a block with
# two blocked regions for pins and Macro -> IO connections.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_liberty Nangate45/Nangate45_fast.lib

read_def testcases/io_constraints6.def

# Run random PPL to incorporate the -exclude constraints into ODB
place_pins -annealing -random -hor_layers metal5 -ver_layer metal6 \
           -exclude right:40-125 \
           -exclude top:10-150

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints6 -halo_width 4.0

set def_file [make_result_file "io_constraints6.def"]
write_def $def_file
diff_files $def_file "io_constraints6.defok"

