# gcd flow pipe cleaner
source "helpers.tcl"
read_lef NangateOpenCellLibrary.lef
read_liberty NangateOpenCellLibrary_typical.lib
read_verilog gcd_nangate45.v
link_design gcd
read_sdc gcd.sdc

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
  -die_area {0 0 100.13 100.8} \
  -core_area {10.07 11.2 90.25 91} \
  -tracks nangate45.tracks

io_placer -random -hor_layer 3 -ver_layer 2

tapcell -endcap_cpp "2" -distance 120 \
  -tapcell_master "FILLCELL_X1" \
  -endcap_master "FILLCELL_X1"

pdngen -verbose gcd_pdn.cfg

# pre-sizing wireload timing
report_checks

global_placement

# resize
set buffer_cell BUF_X4
set_wire_rc -layer metal2
buffer_ports -buffer_cell $buffer_cell
repair_max_cap -buffer_cell $buffer_cell
repair_max_slew -buffer_cell $buffer_cell
repair_max_fanout -max_fanout 100 -buffer_cell $buffer_cell
resize -dont_use {CLKBUF_* AOI211_X1 OAI211_X1}
repair_tie_fanout -max_fanout 100 NangateOpenCellLibrary/LOGIC0_X1/Z
repair_tie_fanout -max_fanout 100 NangateOpenCellLibrary/LOGIC1_X1/Z
repair_hold_violations -buffer_cell $buffer_cell

# missing vsrc file
#analyze_power_grid

clock_tree_synthesis -lut_file nangate45.lut \
  -sol_list nangate45.sol_list \
  -root_buf BUF_X4 \
  -wire_unit 20

set_placement_padding -global -right 8
detailed_placement
check_placement

fastroute -output_file [make_result_file gcd.route_guide] \
          -max_routing_layer 10 \
          -unidirectional_routing true \
          -capacity_adjustment 0.15 \
          -layers_adjustments {{2 0.5} {3 0.5}} \
          -overflow_iterations 200

# detailed route goes here

filler_placement FILL*

# CTS changed the network behind the STA's back.
sta::network_changed
# inlieu of rc extraction
#set_wire_rc -layer metal2

# final report
report_checks -path_delay min_max
report_wns
report_tns
report_check_types -max_slew -violators
report_power

report_floating_nets -verbose
report_design_area

set def_file [make_result_file gcd.def]
write_def $def_file
