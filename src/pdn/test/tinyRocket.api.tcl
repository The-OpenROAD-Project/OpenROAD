source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_lef fakeram45_64x32.lef
read_lef fakeram45_1024x32.lef

read_def tinyRocket/2_5_floorplan_tapcell.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

define_pdn_grid -name grid -starts_with POWER
add_pdn_stripe -layer metal1 -width 0.17 -followpins
add_pdn_stripe -layer metal4 -width 0.48 -pitch 56.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2.0
add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -starts_with POWER -orient {N S FS FN} -pin_direction vertical -halo 4.0 -grid_over_pins
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

define_pdn_grid -macro -starts_with POWER -orient {R90 R270 MXR90 MYR90} -pin_direction horizontal -halo 4.0
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_connect -layers {metal4 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen -verbose

set def_file results/tinyRocket.def
write_def $def_file 

diff_files $def_file tinyRocket.defok
