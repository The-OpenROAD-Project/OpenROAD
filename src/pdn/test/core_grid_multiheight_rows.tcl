# Followpins are generated for every row in the voltage domain with no check
# that the rows are all the same height.  A row spanning an even number of
# standard cell rows carries the SAME net on both of its edges -- see
# sky130_power_switch/power_switch.lef, whose VGND pin has a rect at both y
# edges -- so the "power at one edge, ground at the other" model in
# FollowPins::makeShapes cannot describe it and one of the two straps it emits
# is always on the wrong net.
#
# This floorplan lists its double-height (unithddbl) rows before the
# single-height (unithd) rows they overlap, so the bogus straps are inserted
# first and displace the correct rails.  Nothing is reported, because
# GridComponent::addShape drops a cross-net overlap with only a debug message.
#
# The rails must alternate VSS/VDD on every 2.72um row boundary.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def sky130_multiheight_rows/floorplan.def

add_global_connection -net VDD -pin_pattern {^VPWR$} -power
add_global_connection -net VSS -pin_pattern {^VGND$} -ground
global_connect

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 20.000 -offset 10.000
add_pdn_connect -layers {met1 met4}

pdngen

set def_file [make_result_file core_grid_multiheight_rows.def]
write_def $def_file
diff_files core_grid_multiheight_rows.defok $def_file
