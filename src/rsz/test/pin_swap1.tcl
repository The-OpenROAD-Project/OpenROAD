# Report all swappable pins
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
# just need to read any def
read_def repair_design2.def 

rsz::report_swappable_pins
