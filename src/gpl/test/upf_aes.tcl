source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_aes.v
link_design mpd_top

read_upf -file upf/mpd_aes.upf


set_domain_area PD_AES_1 -area {2.3 2.72 554.3 274.72}
set_domain_area PD_AES_2 -area {2.3 544 554.3 816}


initialize_floorplan -utilization 20 \
                       -aspect_ratio 1 \
                       -core_space 2 \
                       -site unithd

set def_file [make_result_file upf_aes_ifp.def]
write_def $def_file

global_placement -skip_initial_place -density 0.82
set def_file [make_result_file upf_aes_gpl.def]
write_def $def_file






