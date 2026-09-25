# global_route -allow_congestion decides once, when the route finishes,
# that a congested route is accepted. The route stays accepted whatever
# later commands do to the allow_congestion setting: repair_antennas run
# without -allow_congestion, or the setting cleared outright.
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

check "the route is congested: global_route without -allow_congestion fails" {
  catch { global_route }
} 1

global_route -allow_congestion

check "global_route -allow_congestion leaves a route" {
  grt::have_routes
} 1

set congested_guides 0
foreach net [[ord::get_db_block] getNets] {
  foreach guide [$net getGuides] {
    if { [$guide isCongested] } {
      incr congested_guides
    }
  }
}
check "no saved guide is marked as rejected congestion" {
  set congested_guides
} 0

repair_antennas

check "repair_antennas without -allow_congestion keeps the route" {
  grt::have_routes
} 1

grt::set_allow_congestion 0

check "clearing allow_congestion afterwards keeps the route" {
  grt::have_routes
} 1

check "estimate_parasitics -global_routing finds the route" {
  catch { estimate_parasitics -global_routing }
} 0

exit_summary
