# Test if pin access blockage is generated correctly for a case
# with all boundaries blocked except one.
source "helpers.tcl"

# We're not interested in the connections, so don't include the lib.
read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/io_constraints1.v"
link_design "io_constraints1"
read_def "./testcases/io_constraints1.def" -floorplan_initialize

# Run random PPL to incorporate the -exclude constraints into ODB
place_pins -annealing -random -hor_layers metal5 -ver_layer metal6 \
           -exclude left:* -exclude right:* -exclude top:*

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints2 -halo_width 4.0

set def_file [make_result_file io_constraints2.def]
write_def $def_file

diff_files io_constraints2.defok $def_file