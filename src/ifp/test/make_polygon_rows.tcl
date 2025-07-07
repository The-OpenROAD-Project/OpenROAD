# Test make_polygon_rows function for polygonal die areas
# This test creates a polygon die and then uses make_polygon_rows to generate rows
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# Create a polygonal die and generate rows using initialize_floorplan
# This is a hexagonal shape for testing (6 vertices = 12 coordinates)
# set die_polygon  {0 0 0 300 400 300 400 150 300 150 300 0}
# set core_polygon {20 20 20 280 380 280 380 180 280 180 280 20}

# # Die: outer T (unchanged, 8 vertices)
# set die_polygon  {150 0   250 0   250 200   400 200   400 300   0 300   0 200   150 200}

# # Core: inner T entirely inside the die (10 vertices)
# set core_polygon {170 20  230 20  230 180  230 220  380 220  380 280 \
#                   20 280  20 220  170 220  170 180}

# Die polygon – “horizontal” T (original T rotated 90° CCW)
# Die outline — horizontal-T, 8 vertices
set die_polygon  {0 0   100 0   100 150   300 150   300 250   100 250   100 400   0 400}

# Core outline — 20 µm margin inside the die, 8 vertices
set core_polygon {20 20  80 20  80 170  260 170  260 230  80 230  80 380  20 380}


initialize_floorplan -die_polygon $die_polygon \
  -core_polygon $core_polygon \
  -site FreePDK45_38x28_10R_NP_162NW_34O

# set def_file [make_result_file make_polygon_rows.def]
# write_def $def_file
# diff_files make_polygon_rows.defok $def_file
