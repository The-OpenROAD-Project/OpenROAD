# Remove buffers between a net with a track-consuming NDR and a net without
# one: b2 is moved as in remove_buffers7 and n2 (NDR) is merged into n1, and
# n3 is merged into out1 (NDR). The merged nets are rerouted with the NDR of
# the preserved net, and the routing usage must match the routes, so the
# resources each net took with its own NDR are released.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set block [ord::get_db_block]
[$block findInst b2] setLocation 40470 35000

# Tracks each NDR consumes on every layer.
set ndr_tracks [dict create NDR 3]
create_ndr -name NDR -spacing *2 -width *2
assign_ndr -ndr NDR -net n2
assign_ndr -ndr NDR -net out1

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

# Usage of all gcells minus the usage the routes in a segments file take: each
# gcell edge of a wire takes the tracks its net consumes.
proc usage_outside_routes { segs_file } {
  global block ndr_tracks
  set grid [$block getGCellGrid]
  set grid_x [$grid getGridX]
  set usage 0
  foreach layer [[ord::get_db_tech] getLayers] {
    if { [$layer getType] != "ROUTING" } {
      continue
    }
    for { set x 0 } { $x < [llength $grid_x] } { incr x } {
      for { set y 0 } { $y < [llength [$grid getGridY]] } { incr y } {
        set usage [expr { $usage + [$grid getUsage $layer $x $y] }]
      }
    }
  }
  set tile [expr { [lindex $grid_x 1] - [lindex $grid_x 0] }]
  set tracks 1
  set stream [open $segs_file r]
  while { [gets $stream line] >= 0 } {
    if { [llength $line] == 1 && $line ne "(" && $line ne ")" } {
      set ndr [[$block findNet $line] getNonDefaultRule]
      set tracks [expr { $ndr eq "NULL" ? 1 : [dict get $ndr_tracks [$ndr getName]] }]
    } elseif { [llength $line] == 6 } {
      lassign $line x0 y0 layer0 x1 y1 layer1
      if { $layer0 eq $layer1 } {
        set length [expr { abs($x1 - $x0) + abs($y1 - $y0) }]
        set usage [expr { $usage - $length / $tile * $tracks }]
      }
    }
  }
  close $stream
  return $usage
}

global_route

set segs_before [make_result_file "remove_buffers8_before.segs"]
write_global_route_segments $segs_before
set usage_before [usage_outside_routes $segs_before]

global_route -start_incremental
remove_buffers b2 b4
global_route -end_incremental

set segs_file [make_result_file "remove_buffers8.segs"]
write_global_route_segments $segs_file
set usage_after [usage_outside_routes $segs_file]
if { $usage_after == $usage_before } {
  puts "Routing usage matches the routes."
} else {
  puts "Routing usage differs from the routes by [expr { int($usage_after - $usage_before) }]."
}

foreach net_name { n1 out1 } {
  set ndr [[$block findNet $net_name] getNonDefaultRule]
  puts "$net_name NDR: [expr { $ndr eq "NULL" ? "none" : [$ndr getName] }]"
}

diff_files remove_buffers8.segsok $segs_file
