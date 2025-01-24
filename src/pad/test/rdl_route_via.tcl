# Test for RDL router with vias to the pad cells
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads_m9.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

rdl_route -layer metal10 -pad_via via9_0 -width 4 -spacing 4 -allow45 "VDD DVDD VSS DVSS p_*"

set def_file [make_result_file "rdl_route_via.def"]
write_def $def_file
diff_files $def_file "rdl_route_via.defok"
