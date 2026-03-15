# Test that secondary power nets are ordered between primary power and ground.
# With -secondary_power VDDA, the strap group ordering must be:
#   VDD (primary power), VDDA (secondary power), VSS (ground)
# rather than the old broken ordering of VDD, VSS, VDDA.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD  -pin_pattern VDD  -power
add_global_connection -net VSS  -pin_pattern VSS  -ground
add_global_connection -net VDDA -pin_pattern VDDA -power

set_voltage_domain -power VDD -ground VSS -secondary_power VDDA

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

# All three nets participate in the metal4 straps; ordering within each
# pitch group must be VDD, VDDA, VSS (power rails first, ground last).
add_pdn_stripe -layer metal4 -width 0.48 -pitch 15.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -pitch 20.0 -offset 2.0 \
    -nets {VDD VSS}

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

pdngen

set def_file [make_result_file core_grid_secondary_power_ordering.def]
write_def $def_file
diff_files core_grid_secondary_power_ordering.defok $def_file
