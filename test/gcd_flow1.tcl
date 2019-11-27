# gcd flow pipecleaner
read_lef NangateOpenCellLibrary.lef
read_liberty NangateOpenCellLibrary_typical.lib
read_verilog gcd_nangate45.v
link_design gcd
read_sdc gcd.sdc

# Liberty cap/res units per liberty dist units (ff/u and kohm/u for Nangate45)
# These do not look correct based on the LEF
# metal1
# CAPACITANCE CPERSQDIST 7.7161e-05 ; (pf/u^2)
#  EDGECAPACITANCE 2.7365e-05 ; (pf/u)
# cap = 2 edges * 2.7e-5pf/u + 7.7161e-05pf/u^2 *.07u
#     = 5.94e-05 pf/u
#     = 5.94e-2 ff/u
#     = 5.94e-17 f/u
set wire_cap_ff_per_micron 0.00023
set wire_res_kohm_per_micron 0.0016
set resizer_buf_cell BUF_X4
set wire_cap_f_per_micron [expr $wire_cap_ff_per_micron * 1e-15]
set wire_res_ohm_per_micron [expr $wire_res_kohm_per_micron * 1e+3]

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
  -utilization 40 -tracks nangate45.tracks
auto_place_pin metal1

# Pre-sizing wireload timing
report_checks

set_wire_rc -capacitance $wire_cap_ff_per_micron -resistance $wire_res_kohm_per_micron
resize -resize -buffer_cell $resizer_buf_cell

# Pre-placement wireload timing
report_checks

global_placement -timing_driven -wire_cap $wire_cap_f_per_micron \
  -wire_res $wire_res_ohm_per_micron

legalize_placement

report_checks
