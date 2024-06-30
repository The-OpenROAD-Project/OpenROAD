# Test for placing pads outside of the allotted row
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

catch {place_pad -master PADCELL_SIG_H -row IO_SOUTH -location 2830 "IO_SOUTH_SIDE"} err
puts $err
