# Test for RDL router with invalid net list
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

catch {rdl_route -layer metal10 -width 4 -spacing 4 ""} err
puts $err
