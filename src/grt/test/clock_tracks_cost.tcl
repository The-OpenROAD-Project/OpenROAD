# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "clock_route.def"

current_design gcd
create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]
set_propagated_clock [get_clocks {core_clock}]

set guide_file [make_result_file clock_tracks_cost.guide]

set_global_routing_layer_adjustment 2 0.8
set_global_routing_layer_adjustment 3 0.7
set_global_routing_layer_adjustment * 0.5

set_layer_ranges -layers 2-6 -clock_layers 4-6

global_route -clock_tracks_cost 3

write_guides $guide_file

diff_file clock_tracks_cost.guideok $guide_file
