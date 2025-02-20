source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/isolation/mpd_top.v
link_design mpd_top

read_upf -file data/isolation/mpd_top.upf

set_domain_area PD_D1 -area {27 27 60 60}
set_domain_area PD_D2 -area {100 100 180 180}
set_domain_area PD_D3 -area {200 200 300 300}
set_domain_area PD_D4 -area {300 300 400 400}

initialize_floorplan -die_area { 0 0 500 500 } \
    -core_area { 100 100 400 400 } \
    -site unithd

set v_file [make_result_file isolation.v]
write_verilog -include_pwr_gnd $v_file
diff_file $v_file isolation.vok
