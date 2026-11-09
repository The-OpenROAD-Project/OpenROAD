# An incremental reroute that cannot route its congestion away reports it and
# keeps the route, the way CUGR's incremental congestion is already handled.
# It used to abort the command instead, which took the caller down with it:
# repair_timing reroutes through IncrementalParasiticsGuard, so the error
# unwound through a destructor that reroutes again and threw a second time.
#
# Stacking cells on one site is what the flow does before legalizing --
# repair_timing places buffers at their ideal location and GRT reroutes the
# dirty nets before detailed placement moves them apart.  Here it is taken far
# enough that the nets cannot be routed without overflow, so the recovery loop
# runs out of options and has to decide what to report.
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

# -end_incremental deliberately omits -allow_congestion: every global_route
# call decides acceptance for itself, and this is the call that used to fail.
tee -variable incremental_log {
  global_route -end_incremental
}

check "congestion left by the incremental reroute is reported once" {
  llength [regexp -all -inline {WARNING GRT-0128} $incremental_log]
} 1

check "the session's verdict is a warning, not an error" {
  llength [regexp -all -inline {WARNING GRT-0115} $incremental_log]
} 1

check "the reroute does not raise GRT-0232" {
  regexp {GRT-0232} $incremental_log
} 0

check "nor GRT-0116, which end_incremental would reach next" {
  regexp {GRT-0116} $incremental_log
} 0

check "the route survives the incremental reroute" {
  grt::have_routes
} 1

check "accepted congestion is not reported as a rejected route" {
  grt::have_congested_routes
} 0

# The consequence that mattered: a caller holding an
# IncrementalParasiticsGuard finishes its reroute and reads the parasitics
# back, instead of unwinding through the guard's destructor.
check "estimate_parasitics -global_routing accepts the route" {
  catch { estimate_parasitics -global_routing }
} 0

set guide_file [make_result_file "incremental_congestion_is_accepted.guide"]
write_guides $guide_file

check "guides are written for the accepted route" {
  set stream [open $guide_file r]
  set content [read $stream]
  close $stream
  expr { [string length $content] > 0 }
} 1

exit_summary
