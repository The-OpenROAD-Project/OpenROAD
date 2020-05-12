proc report_max_fanout { count } {
  set i 1
  foreach pin [lsort -command pin_fanout_less [get_pins -filter "direction == output" *]] {
    puts " [get_full_name $pin] [pin_fanout $pin]"
    if { $i >= $count } {
      break
    }
    incr i
  }
}

proc pin_fanout { pin } {
  set net [get_net -of_object $pin]
  if { $net != "NULL" } {
    return [llength [get_pins -filter "direction == input" -of_objects $net]]
  } else {
    return 0
  }
}

proc pin_fanout_less { pin1 pin2 } {
  return [expr [pin_fanout $pin1] < [pin_fanout $pin2]]
}
