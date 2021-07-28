source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_def gcd_dual_rails/floorplan.def

# Stdcell power/ground pins
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

# RAM power ground pins
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -name CORE -power VDD  -ground VSS

define_pdn_grid -name main_grid -voltage_domains CORE
add_pdn_stripe -name main_grid -layer metal1 -width 0.17 -followpins
add_pdn_stripe -name main_grid -layer metal2 -width 0.17 -followpins
add_pdn_stripe -name main_grid -layer metal4 -width 0.48 -pitch 56.0 -offset 2 -starts_with POWER
add_pdn_stripe -name main_grid -layer metal7 -width 1.40 -pitch 40.0 -offset 2 -starts_with POWER

add_pdn_connect -name main_grid -layers {metal1 metal2} -cut_pitch 0.16
add_pdn_connect -name main_grid -layers {metal2 metal4}
add_pdn_connect -name main_grid -layers {metal4 metal7}

pdngen -verbose

set def_file results/test_gcd_dual_rails.def
write_def $def_file 

diff_files $def_file test_gcd_dual_rails.defok
