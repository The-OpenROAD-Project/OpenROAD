proc report_max_cap { count } {
  set i 1
  foreach net [lsort -command net_cap_less [get_nets *]] {
    puts " [get_full_name $net] [sta::format_capacitance [net_cap $net] 2]"
    if { $i >= $count } {
      break
    }
    incr i
  }
}

proc net_cap { net } {
  return [$net capacitance rise [sta::find_corner default] max]
}

proc net_cap_less { net1 net2 } {
  return [expr [net_cap $net1] < [net_cap $net2]]
}
