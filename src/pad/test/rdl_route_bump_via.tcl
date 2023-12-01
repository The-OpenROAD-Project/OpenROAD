# Test for RDL router with vias to the bumps
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads_m9.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

rdl_route -layer metal9 -bump_via via9_0 -width 8 -spacing 8 "VDD DVDD VSS DVSS p_*"

set def_file [make_result_file "rdl_route_bump_via.def"]
write_def $def_file
diff_files $def_file "rdl_route_bump_via.defok"
