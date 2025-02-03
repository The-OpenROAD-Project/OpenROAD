# init_floorplan with core area too small for memories
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x32.lef
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x32.lib
read_verilog Nangate45_data/macros.v
link_design RocketTile

catch {initialize_floorplan -die_area "0 0 100.0 100.0" \
  -core_area "20.0 20.0 80.0 80.0" \
  -site FreePDK45_38x28_10R_NP_162NW_34O} err
puts $err
