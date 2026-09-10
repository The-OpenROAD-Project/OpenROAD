# Validates CUGR's per-layer NDR scope: the NDR is defined ONLY on
# met3-met5 (no rule on li1/met1/met2). For the clock nets it is
# assigned to, CUGR should apply the NDR factor on met3-met5 wires
# and via stubs but pay factor 1.0 on met1/met2 wires of the same
# net. This is the behavior FastRoute's 2D phase can't do cleanly,
# and is the reason GRNet::ndr_costs_ is per-layer rather than a
# single net-wide scalar.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "clock_route.def"

current_design gcd
create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]
set_propagated_clock [get_clocks {core_clock}]

# NDR rules cover only met3-met5; li1/met1/met2 are absent on purpose.
create_ndr -name NDR_UPPER \
  -spacing { met3 0.9 met4 0.9 met5 4.8 } \
  -width { met3 0.6 met4 0.6 met5 3.2 }

assign_ndr -ndr NDR_UPPER -net clk
assign_ndr -ndr NDR_UPPER -net clknet_0_clk
assign_ndr -ndr NDR_UPPER -net clknet_2_0__leaf_clk
assign_ndr -ndr NDR_UPPER -net clknet_2_1__leaf_clk
assign_ndr -ndr NDR_UPPER -net clknet_2_2__leaf_clk
assign_ndr -ndr NDR_UPPER -net clknet_2_3__leaf_clk

set guide_file [make_result_file ndr_upper_layers_cugr.guide]

set_global_routing_layer_adjustment met1 0.8
set_global_routing_layer_adjustment met2 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal met1-met5 -clock met3-met5

global_route -verbose -use_cugr

write_guides $guide_file

diff_file ndr_upper_layers_cugr.guideok $guide_file
