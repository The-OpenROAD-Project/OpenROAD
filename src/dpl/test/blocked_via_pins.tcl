# pins under a power via stack column but clear of its metal; the core
# is offset from the origin to exercise the core relative coordinates
source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef blocked_via_pins.lef
read_def blocked_via_pins.def

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

# mid_cell's M3 pin is under the VDD via stack column but away from the
# rail, so it is legal where it is
check_placement

# Mirroring left_cell would reduce wirelength but move its M3 pin onto
# the VDD via stack
optimize_mirroring
check_placement

foreach inst [[ord::get_db_block] getInsts] {
  puts "[$inst getName] [$inst getLocation] [$inst getOrient]"
}
