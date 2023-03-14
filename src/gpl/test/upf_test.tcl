source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_top.v
link_design mpd_top

read_upf -file upf/mpd_top.upf


set_domain_area PD_D1 -area {2.3 2.72 16.1 5.44}
set_domain_area PD_D2 -area {2.3 57.12 16.1 59.84}


initialize_floorplan -utilization 1 \
                       -aspect_ratio 1 \
                       -core_space 2 \
                       -site unithd

set def_file [make_result_file upf_ifp.def]
write_def $def_file

global_placement -skip_initial_place
set def_file [make_result_file upf_gpl.def]
write_def $def_file




