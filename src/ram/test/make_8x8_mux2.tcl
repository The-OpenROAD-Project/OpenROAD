source "helpers.tcl"
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
generate_ram \
  -mask_size 8 \
  -word_size 8 \
  -num_words 8 \
  -column_mux_ratio 2 \
  -read_ports 1 \
  -storage_cell sky130_fd_sc_hd__dfxtp_1 \
  -power_pin VPWR \
  -ground_pin VGND \
  -routing_layer {met1 0.48} \
  -ver_layer {met2 0.48 20} \
  -hor_layer {met3 0.48 10} \
  -filler_cells {sky130_fd_sc_hd__fill_1 sky130_fd_sc_hd__fill_2 \
    sky130_fd_sc_hd__fill_4 sky130_fd_sc_hd__fill_8}
write_verilog [make_result_file test_8x8_mux2.v]

