# The congestion recovery loop an incremental reroute runs when it leaves
# overflow: it pulls in the nets crossing the congested area and reroutes them,
# gives up on a round that changes nothing, legalizes the placement as a last
# resort, and errors if overflow survives all of it.  Stacking cells on one
# site, as repair_timing does before legalizing, is taken far enough here that
# the nets cannot be routed without overflow and every stage runs.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal10 1

set_routing_layers -signal metal2-metal10

global_route -allow_congestion

check "the starting route is congested and accepted" {
  list [grt::has_congestion] [grt::have_routes]
} {1 1}

set_debug_level GRT incr 1

global_route -start_incremental

set block [ord::get_db_block]
set moved 0
foreach inst [$block getInsts] {
  # Fillers and tap cells are FIRM and cannot be moved.
  if { [$inst getPlacementStatus] != "PLACED" } {
    continue
  }
  if { $moved >= 120 } {
    break
  }
  $inst setLocation 70000 70000
  incr moved
}

check "the move dirtied nets to reroute" {
  expr { $moved == 120 }
} 1

# -end_incremental omits -allow_congestion: every call decides acceptance.
set failed [catch {
  tee -variable incremental_log {
    global_route -end_incremental
  }
} error_message]

check "the reroute enters congestion recovery exactly once" {
  llength [regexp -all -inline {entering congestion recovery} $incremental_log]
} 1

check "a round that makes no progress ends the loop early" {
  llength [regexp -all -inline \
    {made no progress -- skipping the remaining} $incremental_log]
} 1

check "the placement legalization runs as the last resort" {
  llength [regexp -all -inline \
    {legalizing the placement as a last resort} $incremental_log]
} 1

check "the summary counts the rounds actually spent" {
  set rounds_logged [llength [regexp -all -inline \
    {congestion recovery round [0-9]+/} $incremental_log]]
  regexp {ended after ([0-9]+) rounds} $incremental_log -> rounds_reported
  list [expr { $rounds_reported == $rounds_logged }] \
    [expr { $rounds_reported < 30 }]
} {1 1}

check "overflow the recovery cannot clear is an error" {
  list $failed $error_message
} {1 GRT-0232}

exit_summary
