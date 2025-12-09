source "helpers.tcl"

set_thread_count [expr [cpu_count] / 4]

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef

generate_ram \
  -bytes_per_word 1 \
  -word_count 8 \
  -read_ports 2 \
  -storage_cell sky130_fd_sc_hd__dlxtp_1 \
  -power_pin VPWR \
  -ground_pin VGND \
  -routing_layer {met1 0.48} \
  -ver_layer {met2 0.48 40} \
  -hor_layer {met3 0.48 20} \
  -filler_cells {sky130_fd_sc_hd__fill_1 sky130_fd_sc_hd__fill_2 \
    sky130_fd_sc_hd__fill_4 sky130_fd_sc_hd__fill_8} \
  -tapcell sky130_fd_sc_hd__tap_1 \
  -max_tap_dist 15

set lef_file [make_result_file make_8x8.lef]
write_abstract_lef $lef_file
diff_files make_8x8.lefok $lef_file

set def_file [make_result_file make_8x8.def]
write_def $def_file
diff_files make_8x8.defok $def_file
