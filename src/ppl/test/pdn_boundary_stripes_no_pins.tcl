# place_pins must not place pins over PDN stripes extended to the die boundary;
# -dont_add_pins leaves the straps without BPins, so avoiding them depends
# entirely on place_pins checking the special net wires (dbSBox)
source "helpers.tcl"
source "pdn_helpers.tcl"

# the available slot count is kept in the log, since it is the main observable
# of the shapes blocking slots; HPWL changes with the pin assignment
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

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

puts "pin to boundary stripe violations: [count_pdn_shape_violations overlap]"
