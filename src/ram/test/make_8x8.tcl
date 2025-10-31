source "helpers.tcl"

set_thread_count {[expr [cpu_count] / 4]}

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef

generate_ram \
  -bytes_per_word 1 \
  -word_count 8 \
  -read_ports 2 \
  -storage_cell sky130_fd_sc_hd__dlxtp_1 \
  -power_net VPWR \
  -ground_net VGND \
  -routing_layer {met1 0.48} \
  -ver_layer {met2 0.48 40} \
  -hor_layer {met3 0.48 20} \
  -filler_cells {sky130_fd_sc_hd__fill_1 sky130_fd_sc_hd__fill_2 \
	  sky130_fd_sc_hd__fill_4 sky130_fd_sc_hd__fill_8}
