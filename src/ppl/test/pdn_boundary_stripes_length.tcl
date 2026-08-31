# place_pins must honor the width-dependent spacing rules to wide boundary
# PDN stripes when long pins have enough parallel run length to trigger them
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal2 -width 2.0 -pitch 20.0 -offset 5.0 \
  -extend_to_boundary
add_pdn_stripe -layer metal3 -width 2.0 -pitch 20.0 -offset 5.0 \
  -extend_to_boundary
add_pdn_connect -layers {metal1 metal2}
add_pdn_connect -layers {metal2 metal3}

pdngen -dont_add_pins

set_pin_length -ver_length 1.0 -hor_length 1.0

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

puts "pin to wide stripe spacing violations: [count_pdn_shape_violations table]"
