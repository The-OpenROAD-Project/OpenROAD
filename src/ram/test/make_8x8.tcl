source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef

generate_ram_netlist \
  -bytes_per_word 1 \
  -word_count 8 \
  -read_ports 2 \
  -storage_cell sky130_fd_sc_hd__dlxtp_1
#    -tristate_cell sky130_fd_sc_hd__ebufn_2
#    -inv_cell sky130_fd_sc_hd__inv_1

ord::design_created

# make_tracks -x_offset 0 -y_offset 0
# set_io_pin_constraint -direction output -region top:*
# set_io_pin_constraint -pin_names {D[0] D[1]} -region top:*
# place_pins -hor_layers met3 -ver_layers met2
#
# filler_placement {sky130_fd_sc_hd__fill_1 sky130_fd_sc_hd__fill_2 \
# 	sky130_fd_sc_hd__fill_4 sky130_fd_sc_hd__fill_8}

set def_file [make_result_file make_8x8.def]
write_def $def_file
diff_files make_8x8.defok $def_file
