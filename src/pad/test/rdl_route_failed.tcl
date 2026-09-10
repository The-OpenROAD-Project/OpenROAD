# Test for RDL router without 45* with failed routes
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

catch { rdl_route -layer metal10 -width 10 -spacing 10 "VDD DVDD VSS DVSS p_*" } error
if { ![string match "*PAD-0007" $error] } {
  puts $error
  error "Expected PAD-0007, got a different failure"
}

puts pass
