# Wire and guide checks shared by the repair_antennas_post_drt_relief tests.

proc guide_geometry { net } {
  set rows {}
  foreach guide [$net getGuides] {
    set box [$guide getBox]
    lappend rows [list [$box xMin] [$box yMin] [$box xMax] [$box yMax] \
      [[$guide getLayer] getName]]
  }
  return [lsort $rows]
}

# Record each routed net's wire and guides before the repair.
proc record_routed_nets { block } {
  global prior_wire prior_guides
  foreach net [$block getNets] {
    if { [$net getWire] ne "NULL" } {
      set prior_wire([$net getName]) [$net getWire]
      set prior_guides([$net getName]) [guide_geometry $net]
    }
  }
}

# A routed net with new guides must have dropped its old wire.
proc report_moved_wires { block } {
  global prior_wire prior_guides
  set rerouted 0
  set stale 0
  foreach net [$block getNets] {
    set name [$net getName]
    if {
      [info exists prior_wire($name)]
      && [guide_geometry $net] ne $prior_guides($name)
    } {
      incr rerouted
      if { [$net getWire] eq $prior_wire($name) } {
        puts "Net $name keeps its old wire"
        incr stale
      }
    }
  }
  puts "Routed nets with new guides: $rerouted, keeping their old wire: $stale"
}
