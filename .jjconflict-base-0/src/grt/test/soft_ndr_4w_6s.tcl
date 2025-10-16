# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
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
set clk [$block findNet clk]

create_ndr -name NDR \
  -spacing { li1 1.02 met1 0.84 met2 0.84 met3 1.8 met4 1.8 met5 9.6 } \
  -width { li1 0.64 met1 0.56 met2 0.56 met3 1.2 met4 1.2 met5 6.4 }

assign_ndr -ndr NDR -net clk
assign_ndr -ndr NDR -net clknet_0_clk
assign_ndr -ndr NDR -net clknet_2_0__leaf_clk
assign_ndr -ndr NDR -net clknet_2_1__leaf_clk
assign_ndr -ndr NDR -net clknet_2_2__leaf_clk
assign_ndr -ndr NDR -net clknet_2_3__leaf_clk

set guide_file [make_result_file soft_ndr_4w_6s.guide]

set_global_routing_layer_adjustment met1 0.5
set_global_routing_layer_adjustment met2 0.5
set_global_routing_layer_adjustment * 0.7

set_routing_layers -signal met1-met5 -clock met3-met5

set_debug GRT "ndrInfo" 1

global_route -verbose

write_guides $guide_file

diff_file soft_ndr_4w_6s.guideok $guide_file
