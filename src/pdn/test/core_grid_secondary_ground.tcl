# Test that -secondary_ground nets are accepted and placed between the
# secondary power nets and the primary ground net.
# Strap group ordering for VDD / VDDA (secondary power) / VSSA (secondary
# ground) / VSS must be: VDD, VDDA, VSSA, VSS.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD  -pin_pattern VDD  -power
add_global_connection -net VSS  -pin_pattern VSS  -ground
add_global_connection -net VDDA -pin_pattern VDDA -power
add_global_connection -net VSSA -pin_pattern VSSA -ground

set_voltage_domain -power VDD -ground VSS \
    -secondary_power VDDA \
    -secondary_ground VSSA

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

# All four nets participate in the metal4 straps.
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2.0 \
    -nets {VDD VSS}

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

pdngen

set def_file [make_result_file core_grid_secondary_ground.def]
write_def $def_file
diff_files core_grid_secondary_ground.defok $def_file
