# Test make_polygon_rows function for polygonal die areas
# This test creates a polygon die and then uses make_polygon_rows to generate rows
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# # Die polygon – “horizontal” T (original T rotated 90° CCW)
# # Die outline — horizontal-T, 8 vertices
set die_area {0 0   100 0   100 150   300 150   300 250   100 250   100 400   0 400}

# Core outline — 20 µm margin inside the die, 8 vertices
set core_area {20 20  80 20  80 170  260 170  260 230  80 230  80 380  20 380}


initialize_floorplan -die_area $die_area \
  -core_area $core_area \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file make_polygon_rows1.def]
write_def $def_file
diff_files make_polygon_rows1.defok $def_file
