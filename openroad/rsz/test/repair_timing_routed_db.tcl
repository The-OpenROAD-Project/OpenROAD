# repair_timing on a detail-routed design keeps the parasitics the flow
# estimated instead of switching to detailed_routing parasitics without
# usable global routes (issue #11852, RSZ-0074). The cells it inserts and
# resizes still land on row sites, as on any routed design.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_routed.def

# A period tight enough for setup violations.
create_clock [get_ports clk] -name core_clock -period 0.43

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Instances whose origin is not on a site of the row at their y.
proc cells_off_row_sites { } {
  set block [ord::get_db_block]
  set rows [dict create]
  foreach row [$block getRows] {
    lassign [$row getOrigin] x y
    dict set rows $y [list $x [$row getSpacing]]
  }
  set off_site 0
  foreach inst [$block getInsts] {
    lassign [$inst getLocation] x y
    if { ![dict exists $rows $y] } {
      incr off_site
      continue
    }
    lassign [dict get $rows $y] row_x spacing
    if { ($x - $row_x) % $spacing != 0 } {
      incr off_site
    }
  }
  return $off_site
}

puts "design_is_routed: [design_is_routed]"
puts "cells off row sites before repair: [cells_off_row_sites]"
repair_timing -setup
puts "parasitics source: [est::parasitics_source]"
puts "cells off row sites after repair: [cells_off_row_sites]"
report_worst_slack -max -digits 3
