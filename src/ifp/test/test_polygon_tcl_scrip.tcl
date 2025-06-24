# Replace your test with the correct coordinates that match the reference file:

source "helpers.tcl"

# 1. Tech & design setup ------------------------------------------------
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

# 2. Die polygon with coordinates that match the reference file
#    Expected DIEAREA: ( 0 0 ) ( 0 300000 ) ( 400000 300000 ) ( 400000 150000 ) ( 300000 150000 ) ( 300000 0 )
#    In microns: (0,0) (0,150) (200,150) (200,75) (150,75) (150,0)
#
#    ^ y
#    |
# 150 +---------+ (200,150)
#     |         |
#  75 |   +-----+ (200,75)
#     |   |
#     +---+--------> x
#   (0,0) 150    200
#
initialize_floorplan -die_polygon "0 0 0 150 200 150 200 75 75 75 75 0" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

# 3. Write & compare DEF ----------------------------------------------
set def_file [make_result_file init_floorplan_polygon.def]
write_def $def_file
diff_files init_floorplan_polygon.defok $def_file