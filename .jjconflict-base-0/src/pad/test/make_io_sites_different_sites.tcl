# Test for making sites for the IO sites
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

make_fake_io_site -name HORIZONTAL -width 50 -height 1
make_fake_io_site -name VERTICAL -width 1 -height 50
make_fake_io_site -name CORNER -width 50 -height 50

# Test make_io_sites
make_io_sites -horizontal_site HORIZONTAL -vertical_site VERTICAL -corner_site CORNER -offset 15

set def_file [make_result_file "make_io_sites_different_sites.def"]
write_def $def_file
diff_files $def_file "make_io_sites_different_sites.defok"
