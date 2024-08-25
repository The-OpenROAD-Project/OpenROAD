# Test for RDL router with 45* with a separate routing for signals
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

rdl_route -layer metal10 -width 6 -spacing 8 -allow45 "VDD DVDD VSS DVSS"
rdl_route -layer metal10 -width 2 -spacing 2 -allow45 "p_*"

set def_file [make_result_file "rdl_route_45_separate.def"]
write_def $def_file
diff_files $def_file "rdl_route_45_separate.defok"
