# Test for connect by abutment
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

connect_by_abutment

set def_file [make_result_file "connect_by_abutment.def"]
write_def $def_file
diff_files $def_file "connect_by_abutment.defok"
