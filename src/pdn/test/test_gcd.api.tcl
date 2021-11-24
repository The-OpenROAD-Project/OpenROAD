source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_def gcd/floorplan.def

add_global_connection -net VDD -pin_pattern {VDD.*} -power
add_global_connection -net VSS -pin_pattern {VSS.*} -ground

define_pdn_grid -name grid -starts_with POWER
add_pdn_stripe -layer metal1 -width 0.17 -pitch  2.4 -offset 0 -followpins
add_pdn_stripe -layer metal4 -width 0.48 -pitch 56.0 -offset 2
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2
add_pdn_connect -layers {metal1 metal4} 
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -orient {R0 R180 MX MY} -pin_direction vertical
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2
add_pdn_connect -layers {metal4 metal5} 
add_pdn_connect -layers {metal5 metal6} 
add_pdn_connect -layers {metal6 metal7}

define_pdn_grid -macro -orient {R90 R270 MXR90 MYR90} -pin_direction horizontal
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2
add_pdn_connect -layers {metal4 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen -verbose

set def_file results/test_gcd.def
write_def $def_file 

diff_files $def_file test_gcd.defok
