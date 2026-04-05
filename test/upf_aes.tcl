source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_aes.v
link_design mpd_top
read_upf -file upf/mpd_aes.upf
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

set_domain_area PD_AES_1 -area {30   30 650 490}
set_domain_area PD_AES_2 -area {30 510 650 970}


initialize_floorplan \
  -die_area {0 0 1000 1000} \
  -core_area {30 30 970 970} \
  -site unithd \
  -additional_site unithddbl

make_tracks

set_routing_layers -signal li1-met5

place_pins \
  -hor_layers met3 \
  -ver_layers met2
global_placement -skip_initial_place -density uniform -routability_driven -timing_driven

detailed_placement -max_displacement 650
improve_placement
check_placement

set def_file [make_result_file upf_aes.def]
write_def $def_file
diff_file $def_file upf_aes.defok
