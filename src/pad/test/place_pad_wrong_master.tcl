# Test for placing pads with the wrong master
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

place_pad -master PADCELL_SIG_V -row IO_EAST -location 500 "u_ddr_ck_p_o"
catch {place_pad -master PADCELL_SIG_H -row IO_WEST -location 600 "u_ddr_cke_o"} err
puts $err
