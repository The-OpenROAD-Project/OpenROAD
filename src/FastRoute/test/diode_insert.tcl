source "helpers.tcl"
read_lef "sky130/sky130_tech.lef"
read_lef "sky130/sky130_std_cell.lef"

read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2

set guide_file [make_result_file diode_insert.guide]
set def_file [make_result_file diode_insert.def]

set_global_routing_layer_adjustment -layer 2 -adjustment 0.0
set_global_routing_layer_adjustment -layer 3 -adjustment 0.15
set_global_routing_layer_adjustment -layer 4 -adjustment 0.15
set_global_routing_layer_adjustment -layer 5 -adjustment 0.15
set_global_routing_layer_adjustment -layer 6 -adjustment 0.15

set_global_routing_layer_pitch -layer 2 -pitch 0.37
set_global_routing_layer_pitch -layer 3 -pitch 0.48
set_global_routing_layer_pitch -layer 4 -pitch 0.74
set_global_routing_layer_pitch -layer 5 -pitch 0.96
set_global_routing_layer_pitch -layer 6 -pitch 3.33

fastroute -max_routing_layer 6 \
          -unidirectional_routing \

repair_antenna -diode_cell_name "sky130_fd_sc_hs__diode_2" \
               -diode_pin_name "DIODE"

set_placement_padding -global -left 0 -right 0
check_placement

write_guides $guide_file
write_def $def_file

diff_file diode_insert.guideok $guide_file
diff_file diode_insert.defok $def_file

