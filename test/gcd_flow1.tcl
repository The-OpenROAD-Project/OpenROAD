# gcd flow pipe cleaner
read_lef NangateOpenCellLibrary.lef
read_liberty NangateOpenCellLibrary_typical.lib
read_verilog gcd_nangate45.v
link_design gcd
read_sdc gcd.sdc

set wire_cap_ff_per_micron 0.00023
set wire_res_kohm_per_micron 0.0016
set resizer_buf_cell BUF_X4

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
  -utilization 40 -tracks nangate45.tracks
auto_place_pin metal1

# Pre-sizing wireload timing
report_checks

set_wire_rc -capacitance $wire_cap_ff_per_micron -resistance $wire_res_kohm_per_micron
resize -resize -buffer_cell $resizer_buf_cell

# Pre-placement wireload timing
report_checks

global_placement -timing_driven

legalize_placement

report_checks
