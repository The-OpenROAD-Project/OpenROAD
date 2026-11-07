# Nested resizer and flow-level routing scopes must not duplicate merge callbacks.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5
set_routing_layers -signal metal2-metal8 -clock metal3-metal8

if { [info exists use_cugr] && $use_cugr } {
  global_route -use_cugr
} else {
  global_route
}
estimate_parasitics -global_routing
set block [ord::get_db_block]

# Undo restores the survivor's guides, so its next rip-up releases route usage.
global_route -start_incremental
odb::dbDatabase_beginEco $block
remove_buffers b1
odb::dbDatabase_undoEco $block
global_route -end_incremental
check "undo restored b1" { expr { [$block findInst b1] ne "NULL" } } 1

global_route -start_incremental
remove_buffers b1
global_route -end_incremental
check "merge removed b1" { expr { [$block findInst b1] eq "NULL" } } 1
check "merge kept n2" { expr { [$block findNet n2] ne "NULL" } } 1

# A duplicated merge charges/releases segments twice and used to underflow here.
global_route -start_incremental
set b2 [$block findInst b2]
lassign [$b2 getLocation] x y
$b2 setLocation [expr { $x + 8000 }] $y
global_route -end_incremental

set segments [make_result_file incremental_nested_callbacks.segs]
write_global_route_segments $segments
check "reroute produced segments" { expr { [file size $segments] > 0 } } 1
exit_summary
