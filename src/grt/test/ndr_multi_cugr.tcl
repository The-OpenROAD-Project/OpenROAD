# Validates CUGR handles multiple distinct NDRs in the same design.
# NDR_THICK (≈2x width) is assigned to half the clock nets; NDR_LITE
# (default width + extra spacing) to the other half. Each net's
# `ndr_costs_` vector must be computed independently from its own
# rule — a single global NDR factor would conflate the two.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "clock_route.def"

current_design gcd
create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]
set_propagated_clock [get_clocks {core_clock}]

# "Thick" NDR: 2x width + 3x spacing
create_ndr -name NDR_THICK \
  -spacing { li1 0.51 met1 0.42 met2 0.42 met3 0.9 met4 0.9 met5 4.8 } \
  -width { li1 0.34 met1 0.28 met2 0.28 met3 0.6 met4 0.6 met5 3.2 }

# "Lite" NDR: default width, only spacing widened ~2x
create_ndr -name NDR_LITE \
  -spacing { li1 0.34 met1 0.28 met2 0.28 met3 0.6 met4 0.6 met5 3.2 } \
  -width { li1 0.17 met1 0.14 met2 0.14 met3 0.3 met4 0.3 met5 1.6 }

# Split the clock tree between the two rules.
assign_ndr -ndr NDR_THICK -net clk
assign_ndr -ndr NDR_THICK -net clknet_0_clk
assign_ndr -ndr NDR_THICK -net clknet_2_0__leaf_clk
assign_ndr -ndr NDR_LITE -net clknet_2_1__leaf_clk
assign_ndr -ndr NDR_LITE -net clknet_2_2__leaf_clk
assign_ndr -ndr NDR_LITE -net clknet_2_3__leaf_clk

set guide_file [make_result_file ndr_multi_cugr.guide]

set_global_routing_layer_adjustment met1 0.8
set_global_routing_layer_adjustment met2 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal met1-met5 -clock met3-met5

global_route -verbose -use_cugr

write_guides $guide_file

diff_file ndr_multi_cugr.guideok $guide_file
