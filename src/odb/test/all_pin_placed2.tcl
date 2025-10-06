# test if all pins are placed
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def design_is_routed1.def

if { [all_pins_placed] } {
  puts "All pins are placed"
} else {
  puts "Some pins are not placed"
}
