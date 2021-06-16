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
  	 	   	 -spacing { li1 0.51 met1 0.42 met2 0.42 met3 0.9 met4 0.9 met5 4.8 } \
		   		 -width { li1 0.17 met1 0.14 met2 0.14 met3 0.3 met4 0.3 met5 1.6 }

assign_ndr -ndr NDR -net clk
assign_ndr -ndr NDR -net clknet_0_clk
assign_ndr -ndr NDR -net clknet_2_0__leaf_clk
assign_ndr -ndr NDR -net clknet_2_1__leaf_clk
assign_ndr -ndr NDR -net clknet_2_2__leaf_clk
assign_ndr -ndr NDR -net clknet_2_3__leaf_clk

set guide_file [make_result_file ndr_1w_3s.guide]

set_global_routing_layer_adjustment met1 0.8
set_global_routing_layer_adjustment met2 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal met1-met5 -clock met3-met5

set_clock_routing -clock_pdrev_fanout 1

global_route

write_guides $guide_file

diff_file ndr_1w_3s.guideok $guide_file
