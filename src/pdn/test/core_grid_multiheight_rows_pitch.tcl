# The followpin pitch must be derived from the standard cell row height, not
# from whichever row happens to come first in the database.  This floorplan
# lists its double-height (unithddbl, 5.44um) rows ahead of the single-height
# (unithd, 2.72um) rows they overlap, so a pitch taken from the first row
# reports 10.88 instead of 5.44.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def sky130_multiheight_rows/floorplan.def

add_global_connection -net VDD -pin_pattern {^VPWR$} -power
add_global_connection -net VSS -pin_pattern {^VGND$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 20.000 -offset 10.000
add_pdn_connect -layers {met1 met4}

pdngen -report_only
