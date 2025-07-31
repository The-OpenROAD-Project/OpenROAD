source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# L shaped die rotated 90 degrees clockwise

catch {
  initialize_floorplan -die_area "0 0 0 150 200 150 200 75 75 75 75 0" \
    -site FreePDK45_38x28_10R_NP_162NW_34O
} error

puts $error
