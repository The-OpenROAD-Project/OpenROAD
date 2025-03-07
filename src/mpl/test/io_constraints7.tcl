# Test if the bundled nets inside annealing are correct for a block with
# pins with different constraint regions and Macro -> IO connections.

#
#
#
# TO DO: This test requires the fix terminal bug fix in order to work!
#
#
#

source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_liberty Nangate45/Nangate45_fast.lib

read_def testcases/io_constraints6.def

set_io_pin_constraint -pin_names {io_1 io_2} -region left:70-90
set_io_pin_constraint -pin_names {io_3} -region right:70-90

# Run random PPL to incorporate the constraints into ODB
place_pins -annealing -random -hor_layers metal5 -ver_layer metal6

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints6 -halo_width 4.0

set def_file [make_result_file "io_constraints7.def"]
write_def $def_file
diff_files $def_file "io_constraints7.defok"

