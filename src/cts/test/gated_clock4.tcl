source "helpers.tcl"

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog gated_clock4.v
link_design -hier multi_sink

initialize_floorplan -die_area "0 0 1000 1000" -core_area "0 0 1000 1000" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
#make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15
source $tracks_file

place_pins -hor_layers $io_placer_hor_layer \
  -ver_layers $io_placer_ver_layer
global_placement -skip_nesterov_place
detailed_placement


source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

create_clock -name core -period 5 clk

set_debug CTS "insertion delay" 1

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -repair_clock_nets

set vout_file [make_result_file gated_clock4.v]
write_verilog $vout_file
diff_files gated_clock4.vok $vout_file
