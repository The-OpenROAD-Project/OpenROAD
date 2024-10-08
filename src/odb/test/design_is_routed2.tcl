source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"
read_def "design_is_routed2.def"

if {![design_is_routed]} {
  error "Design has unrouted nets."
} else {
  puts "Design is fully routed"
}
