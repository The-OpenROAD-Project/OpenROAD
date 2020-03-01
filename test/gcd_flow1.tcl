# gcd flow pipe cleaner
read_lef NangateOpenCellLibrary.lef
read_liberty NangateOpenCellLibrary_typical.lib
read_verilog gcd_nangate45.v
link_design gcd
read_sdc gcd.sdc

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
  -utilization 40 -tracks nangate45.tracks
auto_place_pins metal1

pdngen -verbose gcd_pdn.cfg

# Pre-sizing wireload timing
report_checks

set_wire_rc -layer metal2
resize -resize
repair_max_cap -buffer_cell BUF_X4
repair_max_slew -buffer_cell BUF_X4
repair_max_fanout -max_fanout 100 -buffer_cell BUF_X4
repair_hold_violations -buffer_cell BUF_X4

report_floating_nets -verbose
report_design_area

# Pre-placement wireload timing
report_checks

set_padding -global -right 8
global_placement -timing_driven

legalize_placement

report_checks
