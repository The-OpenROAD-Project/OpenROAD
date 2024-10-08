source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"
read_def "design_is_routed_fail1.def"

if {![design_is_routed -verbose]} {
  catch {error "Design has unrouted nets."} error
} else {
  puts "Design is fully routed"
}

puts $error
