source "helpers.tcl"

set test_name extend_to_core_ring

read_lef ../../../test/Nangate45/Nangate45.lef
read_def extend_to_core_ring/floorplan.def

catch {add_global_connection -net VDD -pin_pattern {^*VDD$}}
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -name CORE -power VDD -ground VSS

define_pdn_grid -name grid \
  -voltage_domains CORE \
  -starts_with POWER

add_pdn_stripe -layer metal1 -width 0.17 -extend_to_core_ring -followpins
add_pdn_stripe -layer metal6 -width 3 -pitch 50 -offset 11.5
add_pdn_ring -layers {metal5 metal6} -widths 5.0 -spacings 3.0 -core_offsets 5
add_pdn_connect -layers {metal1 metal6}
add_pdn_connect -layers {metal5 metal6}

pdngen

set def_file results/extend_to_core_ring.def
write_def $def_file

diff_files $def_file extend_to_core_ring.defok
