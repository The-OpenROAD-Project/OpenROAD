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

add_global_connection -net VDD -pin_pattern {^VPWR$} -power
add_global_connection -net VSS -pin_pattern {^VGND$} -ground
global_connect
set_voltage_domain -power VDD -ground VSS
define_pdn_grid -name ram_grid -voltage_domains {CORE}
add_pdn_stripe -grid ram_grid -layer met1 -followpins -width 0.48
add_pdn_stripe -grid ram_grid -layer met2 -width 0.48 -pitch 45
add_pdn_stripe -grid ram_grid -layer met3 -width 0.48 -pitch 20
add_pdn_connect -layers {met1 met2}
add_pdn_connect -layers {met2 met3}

pdngen

make_tracks -x_offset 0 -y_offset 0
set_io_pin_constraint -direction output -region top:*
set_io_pin_constraint -pin_names {D[*]} -region top:*
place_pins -hor_layers met3 -ver_layers met2

filler_placement {sky130_fd_sc_hd__fill_1 sky130_fd_sc_hd__fill_2 \
	sky130_fd_sc_hd__fill_4 sky130_fd_sc_hd__fill_8}

global_route
detailed_route

set lef_file [make_result_file make_8x8.lef]
write_abstract_lef $lef_file
diff_files make_8x8.lefok $lef_file

set def_file [make_result_file make_8x8.def]
write_def $def_file
diff_files make_8x8.defok $def_file
