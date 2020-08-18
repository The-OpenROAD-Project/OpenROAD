source "helpers.tcl"
read_lef "sky130/sky130_tech.lef"
read_lef "sky130/sky130_std_cell.lef"

read_def "gcd_sky130.def"

current_design gcd
create_clock -name core_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk}]
set_propagated_clock [get_clocks {core_clock}]

set guide_file [make_result_file clock_route.guide]

fastroute -output_file $guide_file \
          -max_routing_layer 6 \
          -unidirectional_routing true \
          -layers_adjustments {{2 0.8} {3 0.7} {4 0.5} {5 0.5} {6 0.5}} \
          -clock_nets_route_flow \
          -min_layer_for_clock_net 4 \

diff_file clock_route.guideok $guide_file
