# A congested global route that was not accepted is reported as congested,
# not as missing, so the error names the cause.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd.def
create_clock [get_ports clk] -name core_clock -period 0.45
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3

set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal10 1

set_routing_layers -signal metal2-metal10

check "before global_route there are no congested routes" {
  grt::have_congested_routes
} 0

check "the route is congested: global_route without -allow_congestion fails" {
  catch { global_route }
} 1

check "a congested route that was not accepted is not usable" {
  grt::have_routes
} 0

check "a congested route that was not accepted is reported" {
  grt::have_congested_routes
} 1

check_error "estimate_parasitics -global_routing says the route is congested" {
  estimate_parasitics -global_routing
} EST-0008

check_error "estimate_parasitics without a flag says the route is congested" {
  estimate_parasitics
} EST-0020

global_route -allow_congestion

check "an accepted route is not reported as congested" {
  grt::have_congested_routes
} 0

check "estimate_parasitics -global_routing uses an accepted route" {
  catch { estimate_parasitics -global_routing }
} 0

# The SPEF a file holds, without its date.
proc spef_body { filename } {
  set stream [open $filename r]
  set body {}
  foreach line [split [read $stream] "\n"] {
    if { ![string match {\*DATE*} $line] } {
      lappend body $line
    }
  }
  close $stream
  return $body
}

set placement_spef [make_result_file congested_routes_placement.spef]
estimate_parasitics -placement -spef_file $placement_spef
set global_routing_spef [make_result_file congested_routes_global_routing.spef]
estimate_parasitics -global_routing -spef_file $global_routing_spef
set auto_spef [make_result_file congested_routes_auto.spef]
estimate_parasitics -spef_file $auto_spef
check "with an accepted route, estimate_parasitics estimates from it" {
  expr { [spef_body $auto_spef] == [spef_body $global_routing_spef] }
} 1
check "the global route estimate is not the placement estimate" {
  expr { [spef_body $auto_spef] == [spef_body $placement_spef] }
} 0
check "the parasitics come from global routing" {
  est::parasitics_source
} global_routing

exit_summary
