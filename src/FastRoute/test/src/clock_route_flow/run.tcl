read_lef "../../input/sky130/input0.lef"
read_lef "../../input/sky130/input1.lef"

read_def "../../input/sky130/input.def"

read_sdc "../../input/sky130/input.sdc"

fastroute -output_file "out.guide" \
          -max_routing_layer 6 \
          -unidirectional_routing true \
          -layers_adjustments {{2 0.8} {3 0.7} {4 0.5} {5 0.5} {6 0.5}} \
          -verbose 2 \
          -clock_nets_route_flow \
          -min_layer_for_clock_net 4 \

exit
