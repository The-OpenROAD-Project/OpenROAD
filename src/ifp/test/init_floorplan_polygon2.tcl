source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# 150 +---------+ (200,150)
#     |         |
#  75 |   +-----+ (200,75)
#     |   |
#     +---+--------> x
#   (0,0) 150    200
#
catch {
initialize_floorplan -die_polygon "0 0 0 150 200 150 200 75 75 75 75 0" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
} error

puts $error