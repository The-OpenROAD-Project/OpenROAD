# Test for RDL router with 45 and no turn penalty
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

rdl_route -layer metal10 -width 6 -spacing 6 -allow45 -turn_penalty 0.0 "VDD DVDD VSS DVSS p_*"

set def_file [make_result_file "rdl_route_45_cost.def"]
write_def $def_file
diff_files $def_file "rdl_route_45_cost.defok"
