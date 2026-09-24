# cell with an M3 pin on top of power via stack metal
source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef blocked_via_layers.lef
read_def blocked_via_layers.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
set_voltage_domain -power VDD -ground VSS
define_pdn_grid -name top
add_pdn_stripe -layer M1 -width 0.018 -followpins
add_pdn_stripe -layer M2 -width 0.018 -followpins
add_pdn_stripe -layer M5 -width 0.12 -spacing 0.072 -pitch 2.7 -offset 1.0
add_pdn_connect -layers {M1 M2}
add_pdn_connect -layers {M2 M5}
pdngen

# m3_cell and unconnected m3_open have M3 pins shorting to VDD M2-M5 via
# stacks; m1_cell is unaffected
catch { check_placement -verbose } error
puts $error

detailed_placement
check_placement

foreach inst [[ord::get_db_block] getInsts] {
  puts "[$inst getName] [$inst getLocation]"
}
