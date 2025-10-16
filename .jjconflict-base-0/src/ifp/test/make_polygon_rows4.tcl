# Test make_polygon_rows function for polygonal die areas
# This test creates a polygon die and then uses make_polygon_rows to generate rows
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# Die: A U-shape with the bottom-left corner at the origin.
set die_area {0 300   400 300   400 0   300 0   300 200   100 200   100 0   0 0}
# Core: An inner U-shape, inset by 20 units from the die boundary.
set core_area {20 280   380 280   380 20   320 20   320 220   80 220   80 20   20 20}

initialize_floorplan -die_area $die_area \
  -core_area $core_area \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file make_polygon_rows4.def]
write_def $def_file
diff_files make_polygon_rows4.defok $def_file
