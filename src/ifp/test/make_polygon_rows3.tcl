# Test make_polygon_rows function for polygonal die areas
# This test creates a polygon die and then uses make_polygon_rows to generate rows
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# This is an L shape for testing - but with invalid coordinates (odd number)
set die_area {0 0  400 300 400 150 300 150 300}
set core_area {20 20 380 280 380 180 280 180 280 20}

catch {
  initialize_floorplan -die_area $die_area \
    -core_area $core_area \
    -site FreePDK45_38x28_10R_NP_162NW_34O
} error

puts $error
