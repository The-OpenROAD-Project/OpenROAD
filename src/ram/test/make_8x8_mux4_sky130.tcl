source "helpers.tcl"

set_thread_count [expr [cpu_count]]

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef

set behavioral_file [make_result_file make_8x8_mux4_behavioral.v]

generate_ram \
  -mask_size 8 \
  -word_size 8 \
  -num_words 8 \
  -column_mux_ratio 4 \
  -read_ports 1 \
  -storage_cell sky130_fd_sc_hd__dfxtp_1 \
  -power_pin VPWR \
  -ground_pin VGND \
  -routing_layer {met1 0.48} \
  -ver_layer {met2 0.48 10} \
  -hor_layer {met3 0.48 7} \
  -filler_cells {sky130_fd_sc_hd__fill_1 sky130_fd_sc_hd__fill_2 \
    sky130_fd_sc_hd__fill_4 sky130_fd_sc_hd__fill_8} \
  -tapcell sky130_fd_sc_hd__tap_1 \
  -max_tap_dist 15 \
  -write_behavioral_verilog $behavioral_file

write_verilog [make_result_file make_8x8_mux4_sky130.v]

set lef_file [make_result_file make_8x8_mux4_sky130.lef]
write_abstract_lef -bloat_occupied_layers $lef_file
diff_files make_8x8_mux4_sky130.lefok $lef_file

set def_file [make_result_file make_8x8_mux4_sky130.def]
write_def $def_file
diff_files make_8x8_mux4_sky130.defok $def_file

diff_files make_8x8_behavioral.vok $behavioral_file
