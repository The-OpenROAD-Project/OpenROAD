source "helpers.tcl"
read_lef "sky130/sky130_tech.lef"
read_lef "sky130/sky130_std_cell.lef"

read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2

set guide_file [make_result_file diode_insert.guide]
set def_file [make_result_file diode_insert.def]

fastroute -output_file $guide_file \
	  -max_routing_layer 6 \
          -unidirectional_routing true \
          -layers_adjustments {{2 0} {3 0.15} {4 0.15} {5 0.15} {6 0.15}} \
          -layers_pitches {{2 0.37} {3 0.48} {4 0.74} {5 0.96} {6 3.33}} \
          -antenna_avoidance_flow -antenna_cell_name "sky130_fd_sc_hs__diode_2" -antenna_pin_name "DIODE" \

set_placement_padding -global -left 0 -right 0
check_placement

write_def $def_file

diff_file diode_insert.guideok $guide_file
diff_file diode_insert.defok $def_file
