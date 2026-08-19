# Test that terminals on different nets block each other during pad placement.
#
# The companion to the unconnected case: both sides carry a net and the nets differ, so
# checkInstancePlacement() must refuse the overlap and the pad is shifted clear.
#
# The bump is centred over one pad but takes its net from a pad at the far end of the row, so the
# two shapes in contact are on different nets by construction.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

# The bump is centred over u_co_0_i, but takes its net from a pad at the far end of the row.
make_io_bump_array -bump DUMMY_BUMP -origin "252.5 2900" -pitch "160 160" -rows 1 -columns 1
assign_io_bump -net p_co_clk_i -terminal u_co_clk_i/PAD BUMP_0_0

place_pads -row IO_NORTH u_v18_25 u_vzz_25 u_co_0_i u_co_1_i u_co_2_i \
  u_vss_11 u_vdd_11 u_co_3_i u_v18_24 u_vzz_24 u_co_4_i u_co_clk_i \
  u_co_tkn_o u_co_v_i u_v18_23 u_vzz_23 u_co_5_i u_co_6_i u_co_7_i \
  u_co_8_i u_v18_22 u_vzz_22 u_vss_10 u_vdd_10 u_clk_A_i u_clk_B_i \
  u_v18_21 u_clk_C_i u_vzz_21 u_clk_o u_vdd_pll u_clk_async_reset_i \
  u_vss_pll u_misc_o u_sel_0_i u_sel_1_i u_vzz_20 u_v18_20 u_sel_2_i \
  u_vss_9 u_vdd_9 u_core_async_reset_i u_ci2_0_o u_ci2_1_o u_v18_19 \
  u_vzz_19 u_ci2_2_o u_ci2_3_o u_ci2_4_o u_ci2_clk_o u_v18_18 u_vzz_18 \
  u_ci2_tkn_i u_ci2_v_o u_vss_8 u_vdd_8 u_ci2_5_o u_ci2_6_o u_v18_17 \
  u_vzz_17

set def_file [make_result_file "order_net_match_different.def"]
write_def $def_file
diff_files $def_file "order_net_match_different.defok"
