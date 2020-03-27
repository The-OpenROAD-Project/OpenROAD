# gcd flow pipe cleaner
source "helpers.tcl"
read_lef NangateOpenCellLibrary.lef
read_liberty NangateOpenCellLibrary_typical.lib
read_verilog gcd_nangate45.v
link_design gcd
read_sdc gcd.sdc

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
  -utilization 30
auto_place_pins metal1

pdngen -verbose gcd_pdn.cfg

# Pre-sizing wireload timing
report_checks

set buffer_cell BUF_X4
set_wire_rc -layer metal2
resize -resize
repair_max_cap -buffer_cell $buffer_cell
repair_max_slew -buffer_cell $buffer_cell
repair_max_fanout -max_fanout 100 -buffer_cell $buffer_cell
repair_hold_violations -buffer_cell $buffer_cell

report_floating_nets -verbose
report_design_area

# Pre-placement wireload timing
report_checks

global_placement -timing_driven

clock_tree_synthesis -lut_file nangate45.lut \
  -sol_list nangate45.sol_list \
  -root_buf BUF_X4 \
  -wire_unit 20

set_placement_padding -global -right 8
detailed_placement
check_placement

filler_placement FILL*

report_checks

set def_file [make_result_file gcd.def]
write_def $def_file
