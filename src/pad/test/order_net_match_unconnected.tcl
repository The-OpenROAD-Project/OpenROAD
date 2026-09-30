# Test that two unconnected terminals block each other during pad placement.
#
# checkInstancePlacement()'s terminal-obstruction branch permits an overlap only when the nets
# match:
#
#     nets_match = term_net == check_net && (check_net != nullptr || term_net != nullptr)
#
# The second clause makes two unconnected terminals block each other even though the first clause
# holds (nullptr == nullptr). No existing test covers it, so rewriting the condition as a plain
# equality leaves the suite green.
#
# Here a bump with no net is centred over a pad whose PAD terminal has been disconnected, so both
# sides are unconnected and the pad must be shifted clear.
#
# See also place_pads_bumps_bump_overlap, which covers the same-net case (overlap permitted).
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

# Take the net off this pad's terminal, so both sides of the check are genuinely unconnected.
set iterm [[[ord::get_db_block] findInst u_co_0_i] findITerm PAD]
$iterm disconnect

# One bump, connected to nothing, centred over that pad.
make_io_bump_array -bump DUMMY_BUMP -origin "252.5 2900" -pitch "160 160" -rows 1 -columns 1

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

set def_file [make_result_file "order_net_match_unconnected.def"]
write_def $def_file
diff_files $def_file "order_net_match_unconnected.defok"
