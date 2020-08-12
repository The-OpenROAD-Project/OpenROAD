read_lef "../../input/sky130/input0.lef"
read_lef "../../input/sky130/input1.lef"

read_def "../../input/sky130/input.def"

set_placement_padding -global -left 2 -right 2

fastroute -output_file "out.guide" \
          -max_routing_layer 6 \
          -unidirectional_routing true \
          -layers_adjustments {{2 0} {3 0.15} {4 0.15} {5 0.15} {6 0.15}} \
          -layers_pitches {{2 0.37} {3 0.48} {4 0.74} {5 0.96} {6 3.33}} \
          -verbose 2 \
          -antenna_avoidance_flow -antenna_cell_name "sky130_fd_sc_hs__diode_2" -antenna_pin_name "DIODE" \

set_placement_padding -global -left 0 -right 0
check_placement

write_def "out.def"

exit
