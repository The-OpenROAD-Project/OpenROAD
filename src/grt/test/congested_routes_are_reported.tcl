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

global_route -allow_congestion

check "an accepted route is not reported as congested" {
  grt::have_congested_routes
} 0

check "estimate_parasitics -global_routing uses an accepted route" {
  catch { estimate_parasitics -global_routing }
} 0

exit_summary
