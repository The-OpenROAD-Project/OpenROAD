source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/mpd_top/mpd_top.v
link_design mpd_top

read_upf -file data/mpd_top/mpd_top_combined.upf

set upf_file [make_result_file write.upf]
write_upf $upf_file
diff_file $upf_file write.upfok
