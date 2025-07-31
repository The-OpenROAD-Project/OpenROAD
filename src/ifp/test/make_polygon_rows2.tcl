# Test make_polygon_rows function for polygonal die areas
# This test creates a polygon die and then uses make_polygon_rows to generate rows
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# Die: outer T (unchanged, 8 vertices)
set die_area {150 0   250 0   250 200   400 200\
             400 300   0 300   0 200   150 200}

# Core: inner T entirely inside the die (8 vertices)
set core_area {170 20  230 20  230 220  380 220\
              380 280  20 280  20 220  170 220}

initialize_floorplan -die_area $die_area \
  -core_area $core_area \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file make_polygon_rows2.def]
write_def $def_file
diff_files make_polygon_rows2.defok $def_file
