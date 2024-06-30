# make_tracks -x/y_offset > die width/height
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 200 200" \
  -core_area "10 10 190 190" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

if { [catch {make_tracks metal2 -x_offset 300 -x_pitch 0.2 \
               -y_offset .1 -y_pitch 0.2} error ] } {
  puts $error
}

if { [catch {make_tracks metal2 -x_offset 0.1 -x_pitch 0.2 \
               -y_offset 300 -y_pitch 0.2} error ] } {
  puts $error
}
