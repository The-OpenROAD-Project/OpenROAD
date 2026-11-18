# Test for making sites for the corner IO sites with initial rotation
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test make_io_sites
make_io_sites \
  -horizontal_site IOSITE \
  -vertical_site IOSITE \
  -corner_site IOSITE \
  -offset 15 \
  -rotation_corner MY

set def_file [make_result_file "make_corner_sites.def"]
write_def $def_file
diff_files $def_file "make_corner_sites.defok"
