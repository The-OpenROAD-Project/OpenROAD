# Test for placing pads using place_pads with uniform spacing
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

# Make bump array
make_io_bump_array -bump DUMMY_BUMP -origin "210.0 215.0" -pitch "160 160" -rows 17 -columns 17
remove_io_bump BUMP_5_8
remove_io_bump BUMP_6_8
remove_io_bump BUMP_7_8
remove_io_bump BUMP_8_5
remove_io_bump BUMP_8_6
remove_io_bump BUMP_8_7
remove_io_bump BUMP_8_8
remove_io_bump BUMP_8_9
remove_io_bump BUMP_8_10
remove_io_bump BUMP_8_11
remove_io_bump BUMP_9_8
remove_io_bump BUMP_10_8
remove_io_bump BUMP_11_8

######## Assign Bumps ########
assign_io_bump -net p_ddr_dm_1_o -terminal u_ddr_dm_1_o/PAD BUMP_0_0
assign_io_bump -net p_ddr_dqs_n_1_io -terminal u_ddr_dqs_n_1_io/PAD BUMP_1_1

assign_io_bump -net p_ddr_dqs_p_1_io -terminal u_ddr_dqs_p_1_io/PAD BUMP_2_0

assign_io_bump -net p_ddr_ba_2_o -terminal u_ddr_ba_2_o/PAD BUMP_3_1
assign_io_bump -net p_ddr_ba_1_o -terminal u_ddr_ba_1_o/PAD BUMP_3_0
assign_io_bump -net p_ddr_ba_0_o -terminal u_ddr_ba_0_o/PAD BUMP_3_2

assign_io_bump -net p_ddr_addr_15_o -terminal u_ddr_addr_15_o/PAD BUMP_4_0
assign_io_bump -net p_ddr_addr_14_o -terminal u_ddr_addr_14_o/PAD BUMP_4_2
assign_io_bump -net p_ddr_addr_13_o -terminal u_ddr_addr_13_o/PAD BUMP_4_4
assign_io_bump -net p_ddr_addr_12_o -terminal u_ddr_addr_12_o/PAD BUMP_5_3

assign_io_bump -net p_ddr_addr_11_o -terminal u_ddr_addr_11_o/PAD BUMP_5_2
assign_io_bump -net p_ddr_addr_10_o -terminal u_ddr_addr_10_o/PAD BUMP_5_4

assign_io_bump -net p_ddr_addr_9_o -terminal u_ddr_addr_9_o/PAD BUMP_6_0
assign_io_bump -net p_ddr_addr_8_o -terminal u_ddr_addr_8_o/PAD BUMP_6_2

assign_io_bump -net p_ddr_addr_7_o -terminal u_ddr_addr_7_o/PAD BUMP_7_1
assign_io_bump -net p_ddr_addr_6_o -terminal u_ddr_addr_6_o/PAD BUMP_7_0
assign_io_bump -net p_ddr_addr_5_o -terminal u_ddr_addr_5_o/PAD BUMP_7_2
assign_io_bump -net p_ddr_addr_4_o -terminal u_ddr_addr_4_o/PAD BUMP_7_4

assign_io_bump -net p_ddr_addr_3_o -terminal u_ddr_addr_3_o/PAD BUMP_8_0
assign_io_bump -net p_ddr_addr_2_o -terminal u_ddr_addr_2_o/PAD BUMP_8_2
assign_io_bump -net p_ddr_addr_1_o -terminal u_ddr_addr_1_o/PAD BUMP_8_4

assign_io_bump -net p_ddr_addr_0_o -terminal u_ddr_addr_0_o/PAD BUMP_9_0

assign_io_bump -net p_ddr_odt_o -terminal u_ddr_odt_o/PAD BUMP_10_3
assign_io_bump -net p_ddr_reset_n_o -terminal u_ddr_reset_n_o/PAD BUMP_10_1
assign_io_bump -net p_ddr_we_n_o -terminal u_ddr_we_n_o/PAD BUMP_10_0
assign_io_bump -net p_ddr_cas_n_o -terminal u_ddr_cas_n_o/PAD BUMP_10_2

assign_io_bump -net p_ddr_ras_n_o -terminal u_ddr_ras_n_o/PAD BUMP_11_1
assign_io_bump -net p_ddr_cs_n_o -terminal u_ddr_cs_n_o/PAD BUMP_11_0
assign_io_bump -net p_ddr_cke_o -terminal u_ddr_cke_o/PAD BUMP_11_2
assign_io_bump -net p_ddr_ck_n_o -terminal u_ddr_ck_n_o/PAD BUMP_11_4

assign_io_bump -net p_ddr_ck_p_o -terminal u_ddr_ck_p_o/PAD BUMP_13_1
assign_io_bump -net p_ddr_dqs_n_2_io -terminal u_ddr_dqs_n_2_io/PAD BUMP_13_0
assign_io_bump -net p_ddr_dqs_p_2_io -terminal u_ddr_dqs_p_2_io/PAD BUMP_13_2
assign_io_bump -net p_ddr_dm_2_o -terminal u_ddr_dm_2_o/PAD BUMP_14_1

assign_io_bump -net p_ddr_dq_23_io -terminal u_ddr_dq_23_io/PAD BUMP_16_0
assign_io_bump -net p_ddr_dq_22_io -terminal u_ddr_dq_22_io/PAD BUMP_15_1
assign_io_bump -net p_ddr_dq_21_io -terminal u_ddr_dq_21_io/PAD BUMP_16_1

assign_io_bump -net p_ddr_dq_20_io -terminal u_ddr_dq_20_io/PAD BUMP_14_2

assign_io_bump -net p_ddr_dq_19_io -terminal u_ddr_dq_19_io/PAD BUMP_16_3
assign_io_bump -net p_ddr_dq_18_io -terminal u_ddr_dq_18_io/PAD BUMP_14_3
assign_io_bump -net p_ddr_dq_17_io -terminal u_ddr_dq_17_io/PAD BUMP_13_4
assign_io_bump -net p_ddr_dq_16_io -terminal u_ddr_dq_16_io/PAD BUMP_15_4

assign_io_bump -net p_ddr_dq_31_io -terminal u_ddr_dq_31_io/PAD BUMP_12_4
assign_io_bump -net p_ddr_dq_30_io -terminal u_ddr_dq_30_io/PAD BUMP_13_5
assign_io_bump -net p_ddr_dq_29_io -terminal u_ddr_dq_29_io/PAD BUMP_15_5
assign_io_bump -net p_ddr_dq_28_io -terminal u_ddr_dq_28_io/PAD BUMP_16_5

assign_io_bump -net p_ddr_dq_27_io -terminal u_ddr_dq_27_io/PAD BUMP_13_6

assign_io_bump -net p_ddr_dq_26_io -terminal u_ddr_dq_26_io/PAD BUMP_14_6
assign_io_bump -net p_ddr_dq_25_io -terminal u_ddr_dq_25_io/PAD BUMP_12_6
assign_io_bump -net p_ddr_dq_24_io -terminal u_ddr_dq_24_io/PAD BUMP_13_7

assign_io_bump -net p_ddr_dqs_n_3_io -terminal u_ddr_dqs_n_3_io/PAD BUMP_14_7
assign_io_bump -net p_ddr_dqs_p_3_io -terminal u_ddr_dqs_p_3_io/PAD BUMP_12_7
assign_io_bump -net p_ddr_dm_3_o -terminal u_ddr_dm_3_o/PAD BUMP_13_8
assign_io_bump -net p_bsg_tag_clk_i -terminal u_bsg_tag_clk_i/PAD BUMP_15_8

assign_io_bump -net p_bsg_tag_data_i -terminal u_bsg_tag_data_i/PAD BUMP_12_8
assign_io_bump -net p_bsg_tag_en_i -terminal u_bsg_tag_en_i/PAD BUMP_13_9
assign_io_bump -net p_ci_8_i -terminal u_ci_8_i/PAD BUMP_15_9

assign_io_bump -net p_ci_7_i -terminal u_ci_7_i/PAD BUMP_12_9

assign_io_bump -net p_ci_6_i -terminal u_ci_6_i/PAD BUMP_16_10
assign_io_bump -net p_ci_5_i -terminal u_ci_5_i/PAD BUMP_14_10
assign_io_bump -net p_ci_v_i -terminal u_ci_v_i/PAD BUMP_12_10
assign_io_bump -net p_ci_tkn_o -terminal u_ci_tkn_o/PAD BUMP_13_11

assign_io_bump -net p_ci_clk_i -terminal u_ci_clk_i/PAD BUMP_14_11
assign_io_bump -net p_ci_4_i -terminal u_ci_4_i/PAD BUMP_12_11
assign_io_bump -net p_ci_3_i -terminal u_ci_3_i/PAD BUMP_13_12
assign_io_bump -net p_ci_2_i -terminal u_ci_2_i/PAD BUMP_15_12

assign_io_bump -net p_ci_1_i -terminal u_ci_1_i/PAD BUMP_15_13

assign_io_bump -net p_ci_0_i -terminal u_ci_0_i/PAD BUMP_15_14
assign_io_bump -net p_ci2_8_o -terminal u_ci2_8_o/PAD BUMP_16_14
assign_io_bump -net p_ci2_7_o -terminal u_ci2_7_o/PAD BUMP_16_15

assign_io_bump -net p_ci2_6_o -terminal u_ci2_6_o/PAD BUMP_15_16
assign_io_bump -net p_ci2_5_o -terminal u_ci2_5_o/PAD BUMP_14_15

assign_io_bump -net p_ci2_v_o -terminal u_ci2_v_o/PAD BUMP_13_13
assign_io_bump -net p_ci2_tkn_i -terminal u_ci2_tkn_i/PAD BUMP_13_15

assign_io_bump -net p_ci2_clk_o -terminal u_ci2_clk_o/PAD BUMP_12_13
assign_io_bump -net p_ci2_4_o -terminal u_ci2_4_o/PAD BUMP_12_15
assign_io_bump -net p_ci2_3_o -terminal u_ci2_3_o/PAD BUMP_12_16
assign_io_bump -net p_ci2_2_o -terminal u_ci2_2_o/PAD BUMP_12_14

assign_io_bump -net p_ci2_1_o -terminal u_ci2_1_o/PAD BUMP_11_15
assign_io_bump -net p_ci2_0_o -terminal u_ci2_0_o/PAD BUMP_11_16
assign_io_bump -net p_core_async_reset_i -terminal u_core_async_reset_i/PAD BUMP_11_14

assign_io_bump -net p_sel_2_i -terminal u_sel_2_i/PAD BUMP_10_15

assign_io_bump -net p_sel_1_i -terminal u_sel_1_i/PAD BUMP_10_12
assign_io_bump -net p_sel_0_i -terminal u_sel_0_i/PAD BUMP_9_13
assign_io_bump -net p_misc_o -terminal u_misc_o/PAD BUMP_9_15

assign_io_bump -net p_clk_async_reset_i -terminal u_clk_async_reset_i/PAD BUMP_9_14

assign_io_bump -net p_clk_o -terminal u_clk_o/PAD BUMP_8_13

assign_io_bump -net p_clk_C_i -terminal u_clk_C_i/PAD BUMP_8_16

assign_io_bump -net p_clk_B_i -terminal u_clk_B_i/PAD BUMP_8_12
assign_io_bump -net p_clk_A_i -terminal u_clk_A_i/PAD BUMP_7_13

assign_io_bump -net p_co_8_i -terminal u_co_8_i/PAD BUMP_6_13
assign_io_bump -net p_co_7_i -terminal u_co_7_i/PAD BUMP_6_15
assign_io_bump -net p_co_6_i -terminal u_co_6_i/PAD BUMP_6_16
assign_io_bump -net p_co_5_i -terminal u_co_5_i/PAD BUMP_6_14

assign_io_bump -net p_co_v_i -terminal u_co_v_i/PAD BUMP_5_15
assign_io_bump -net p_co_tkn_o -terminal u_co_tkn_o/PAD BUMP_5_16
assign_io_bump -net p_co_clk_i -terminal u_co_clk_i/PAD BUMP_5_14
assign_io_bump -net p_co_4_i -terminal u_co_4_i/PAD BUMP_5_12

assign_io_bump -net p_co_3_i -terminal u_co_3_i/PAD BUMP_4_16

assign_io_bump -net p_co_2_i -terminal u_co_2_i/PAD BUMP_3_16
assign_io_bump -net p_co_1_i -terminal u_co_1_i/PAD BUMP_3_14
assign_io_bump -net p_co_0_i -terminal u_co_0_i/PAD BUMP_2_15

assign_io_bump -net p_co2_8_o -terminal u_co2_8_o/PAD BUMP_0_16
assign_io_bump -net p_co2_7_o -terminal u_co2_7_o/PAD BUMP_1_15
assign_io_bump -net p_co2_6_o -terminal u_co2_6_o/PAD BUMP_0_15
assign_io_bump -net p_co2_5_o -terminal u_co2_5_o/PAD BUMP_1_14

assign_io_bump -net p_co2_v_o -terminal u_co2_v_o/PAD BUMP_0_13
assign_io_bump -net p_co2_tkn_i -terminal u_co2_tkn_i/PAD BUMP_2_13
assign_io_bump -net p_co2_clk_o -terminal u_co2_clk_o/PAD BUMP_3_12
assign_io_bump -net p_co2_4_o -terminal u_co2_4_o/PAD BUMP_1_12

assign_io_bump -net p_co2_3_o -terminal u_co2_3_o/PAD BUMP_4_12
assign_io_bump -net p_co2_2_o -terminal u_co2_2_o/PAD BUMP_3_11
assign_io_bump -net p_co2_1_o -terminal u_co2_1_o/PAD BUMP_1_11
assign_io_bump -net p_co2_0_o -terminal u_co2_0_o/PAD BUMP_0_11

assign_io_bump -net p_bsg_tag_clk_o -terminal u_bsg_tag_clk_o/PAD BUMP_3_10

assign_io_bump -net p_bsg_tag_data_o -terminal u_bsg_tag_data_o/PAD BUMP_2_10
assign_io_bump -net p_ddr_dq_7_io -terminal u_ddr_dq_7_io/PAD BUMP_4_10
assign_io_bump -net p_ddr_dq_6_io -terminal u_ddr_dq_6_io/PAD BUMP_3_9

assign_io_bump -net p_ddr_dq_5_io -terminal u_ddr_dq_5_io/PAD BUMP_2_9
assign_io_bump -net p_ddr_dq_4_io -terminal u_ddr_dq_4_io/PAD BUMP_4_9
assign_io_bump -net p_ddr_dq_3_io -terminal u_ddr_dq_3_io/PAD BUMP_3_8
assign_io_bump -net p_ddr_dq_2_io -terminal u_ddr_dq_2_io/PAD BUMP_1_8

assign_io_bump -net p_ddr_dq_1_io -terminal u_ddr_dq_1_io/PAD BUMP_4_8
assign_io_bump -net p_ddr_dq_0_io -terminal u_ddr_dq_0_io/PAD BUMP_3_7

assign_io_bump -net p_ddr_dm_0_o -terminal u_ddr_dm_0_o/PAD BUMP_2_7
assign_io_bump -net p_ddr_dqs_n_0_io -terminal u_ddr_dqs_n_0_io/PAD BUMP_4_7

assign_io_bump -net p_ddr_dqs_p_0_io -terminal u_ddr_dqs_p_0_io/PAD BUMP_0_6
assign_io_bump -net p_ddr_dq_15_io -terminal u_ddr_dq_15_io/PAD BUMP_2_6
assign_io_bump -net p_ddr_dq_14_io -terminal u_ddr_dq_14_io/PAD BUMP_4_6
assign_io_bump -net p_ddr_dq_13_io -terminal u_ddr_dq_13_io/PAD BUMP_3_5

assign_io_bump -net p_ddr_dq_12_io -terminal u_ddr_dq_12_io/PAD BUMP_2_5
assign_io_bump -net p_ddr_dq_11_io -terminal u_ddr_dq_11_io/PAD BUMP_4_5
assign_io_bump -net p_ddr_dq_10_io -terminal u_ddr_dq_10_io/PAD BUMP_3_4
assign_io_bump -net p_ddr_dq_9_io -terminal u_ddr_dq_9_io/PAD BUMP_1_4
assign_io_bump -net p_ddr_dq_8_io -terminal u_ddr_dq_8_io/PAD BUMP_0_4

# Place pads
place_pads -row IO_EAST \
    u_ddr_dq_23_io u_ddr_dq_22_io u_ddr_dq_21_io u_vdd_4 u_vss_4 u_ddr_dq_20_io u_vzz_9 \
    u_v18_9 u_ddr_dq_19_io u_ddr_dq_18_io u_ddr_dq_17_io u_ddr_dq_16_io u_vzz_10 u_v18_10 \
    u_ddr_dq_31_io u_ddr_dq_30_io u_ddr_dq_29_io u_ddr_dq_28_io u_vzz_11 u_v18_11 u_ddr_dq_27_io \
    u_vdd_5 u_vss_5 u_ddr_dq_26_io u_ddr_dq_25_io u_ddr_dq_24_io u_vzz_12 u_v18_12 u_ddr_dqs_n_3_io \
    u_ddr_dqs_p_3_io u_ddr_dm_3_o u_bsg_tag_clk_i u_vzz_13 u_v18_13 u_bsg_tag_data_i u_bsg_tag_en_i \
    u_ci_8_i u_vdd_6 u_vss_6 u_ci_7_i u_vzz_14 u_v18_14 u_ci_6_i u_ci_5_i u_ci_v_i u_ci_tkn_o u_vzz_15 \
    u_v18_15 u_ci_clk_i u_ci_4_i u_ci_3_i u_ci_2_i u_vzz_16 u_v18_16 u_ci_1_i u_vdd_7 u_vss_7 u_ci_0_i \
    u_ci2_8_o u_ci2_7_o
place_pads -row IO_WEST u_vss_15 u_vdd_15 u_ddr_dq_8_io u_ddr_dq_9_io u_ddr_dq_10_io u_ddr_dq_11_io \
    u_ddr_dq_12_io u_v18_32 u_vzz_32 u_ddr_dq_13_io u_ddr_dq_14_io u_ddr_dq_15_io u_ddr_dqs_p_0_io \
    u_v18_31 u_vzz_31 u_ddr_dqs_n_0_io u_ddr_dm_0_o u_vss_14 u_vdd_14 u_ddr_dq_0_io u_ddr_dq_1_io \
    u_v18_30 u_vzz_30 u_ddr_dq_2_io u_ddr_dq_3_io u_ddr_dq_4_io u_ddr_dq_5_io u_v18_29 u_vzz_29 \
    u_ddr_dq_6_io u_ddr_dq_7_io u_bsg_tag_data_o u_vss_13 u_vdd_13 u_bsg_tag_clk_o u_v18_28 u_vzz_28 \
    u_co2_0_o u_co2_1_o u_co2_2_o u_co2_3_o u_v18_27 u_vzz_27 u_co2_4_o u_co2_clk_o u_co2_tkn_i \
    u_co2_v_o u_v18_26 u_vzz_26 u_vss_12 u_vdd_12 u_co2_5_o u_co2_6_o u_co2_7_o u_co2_8_o
place_pads -row IO_SOUTH u_ddr_dm_1_o u_ddr_dqs_n_1_io u_vzz_0 u_ddr_dqs_p_1_io u_vss_0 u_ddr_ba_2_o \
    u_ddr_ba_1_o u_ddr_ba_0_o u_vzz_1 u_v18_1 u_ddr_addr_15_o u_ddr_addr_14_o u_ddr_addr_13_o \
    u_ddr_addr_12_o u_vzz_2 u_v18_2 u_ddr_addr_11_o u_ddr_addr_10_o u_vdd_1 u_vss_1 u_ddr_addr_9_o \
    u_ddr_addr_8_o u_vzz_3 u_v18_3 u_ddr_addr_7_o u_ddr_addr_6_o u_ddr_addr_5_o u_ddr_addr_4_o \
    u_vzz_4 u_v18_4 u_ddr_addr_3_o u_ddr_addr_2_o u_ddr_addr_1_o u_vdd_2 u_vss_2 u_ddr_addr_0_o \
    u_vzz_5 u_v18_5 u_ddr_odt_o u_ddr_reset_n_o u_ddr_we_n_o u_ddr_cas_n_o u_vzz_6 u_v18_6 u_ddr_ras_n_o \
    u_ddr_cs_n_o u_ddr_cke_o u_ddr_ck_n_o u_vzz_7 u_v18_7 u_vdd_3 u_vss_3 u_ddr_ck_p_o u_ddr_dqs_n_2_io \
    u_ddr_dqs_p_2_io u_ddr_dm_2_o u_vzz_8 u_v18_8

set insts [list u_v18_25 u_vzz_25 u_co_0_i u_co_1_i u_co_2_i u_vss_11 u_vdd_11 u_co_3_i \
    u_v18_24 u_vzz_24 u_co_4_i u_co_clk_i u_co_tkn_o u_co_v_i u_v18_23 u_vzz_23 u_co_5_i u_co_6_i \
    u_co_7_i u_co_8_i u_v18_22 u_vzz_22 u_vss_10 u_vdd_10 u_clk_A_i u_clk_B_i u_v18_21 u_clk_C_i \
    u_vzz_21 u_clk_o u_vdd_pll u_clk_async_reset_i u_vss_pll u_misc_o u_sel_0_i u_sel_1_i u_vzz_20 \
    u_v18_20 u_sel_2_i u_vss_9 u_vdd_9 u_core_async_reset_i u_ci2_0_o u_ci2_1_o u_v18_19 u_vzz_19 \
    u_ci2_2_o u_ci2_3_o u_ci2_4_o u_ci2_clk_o u_v18_18 u_vzz_18 u_ci2_tkn_i u_ci2_v_o u_vss_8 u_vdd_8 \
    u_ci2_5_o u_ci2_6_o u_v18_17 u_vzz_17]
place_pads -row IO_NORTH $insts


set def_file [make_result_file "place_pads_bumps.def"]
write_def $def_file
diff_files $def_file "place_pads_bumps.defok"
