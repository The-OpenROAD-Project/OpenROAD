# Test for RDL router without 45* with failed routes
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

catch {rdl_route -layer metal10 -width 10 -spacing 10 -max_iterations 4 "VDD DVDD VSS DVSS p_*"} error
puts $error
