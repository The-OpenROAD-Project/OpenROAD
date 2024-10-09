# Test for RDL router without 45*
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

odb::dbNet_create [ord::get_db_block] "test_net"
assign_io_bump -net test_net BUMP_5_11

rdl_route -layer metal10 -width 4 -spacing 4 "test_net p_*"

set def_file [make_result_file "rdl_route_single_target.def"]
write_def $def_file
diff_files $def_file "rdl_route_single_target.defok"
