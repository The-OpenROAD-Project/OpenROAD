# Exercises the escalating soft-NDR overflow-loop escape valve.
#
# Aggressive custom NDRs on every clock net, with tightened routing resources,
# force global routing to disable congested NDR nets to relieve overflow.
# Disabling NDR nets a few at a time with a full overflow-loop restart per net
# is O(N) restarts. To bound the restarts, the escape valve escalates the batch
# size: the first restart disables half of the congested NDR nets and any
# subsequent restart disables all of the remaining ones, so the loop terminates
# in at most two restarts. This test checks that the flow completes and produces
# a stable result under that condition.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "clock_route.def"

current_design gcd
create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]
set_propagated_clock [get_clocks {core_clock}]

set db [ord::get_db]
set block [[$db getChip] getBlock]

create_ndr -name CAPNDR \
  -spacing { li1 1.02 met1 0.84 met2 0.84 met3 1.8 met4 1.8 met5 9.6 } \
  -width { li1 0.64 met1 0.56 met2 0.56 met3 1.2 met4 1.2 met5 6.4 }

# Apply the NDR to every clock net so several must be disabled under congestion.
foreach net [$block getNets] {
  if { [$net getSigType] == "CLOCK" } {
    assign_ndr -ndr CAPNDR -net [$net getName]
  }
}

set_global_routing_layer_adjustment met1 0.6
set_global_routing_layer_adjustment met2 0.6
set_global_routing_layer_adjustment * 0.7

set_routing_layers -signal met1-met5 -clock met3-met4

global_route -verbose -allow_congestion

set guide_file [make_result_file soft_ndr_escalation.guide]
write_guides $guide_file

diff_file soft_ndr_escalation.guideok $guide_file
