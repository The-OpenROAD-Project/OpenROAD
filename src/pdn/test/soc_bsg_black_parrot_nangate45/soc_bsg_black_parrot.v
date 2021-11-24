
module bsg_black_parrot (
  bsg_tag_clk_i,
  bsg_tag_clk_o,
  bsg_tag_data_i,
  bsg_tag_data_o,
  bsg_tag_en_i,
  ci2_0_o,
  ci2_1_o,
  ci2_2_o,
  ci2_3_o,
  ci2_4_o,
  ci2_5_o,
  ci2_6_o,
  ci2_7_o,
  ci2_8_o,
  ci2_clk_o,
  ci2_tkn_i,
  ci2_v_o,
  ci_0_i,
  ci_1_i,
  ci_2_i,
  ci_3_i,
  ci_4_i,
  ci_5_i,
  ci_6_i,
  ci_7_i,
  ci_8_i,
  ci_clk_i,
  ci_tkn_o,
  ci_v_i,
  clk_A_i,
  clk_B_i,
  clk_C_i,
  clk_async_reset_i,
  clk_o,
  co2_0_o,
  co2_1_o,
  co2_2_o,
  co2_3_o,
  co2_4_o,
  co2_5_o,
  co2_6_o,
  co2_7_o,
  co2_8_o,
  co2_clk_o,
  co2_tkn_i,
  co2_v_o,
  co_0_i,
  co_1_i,
  co_2_i,
  co_3_i,
  co_4_i,
  co_5_i,
  co_6_i,
  co_7_i,
  co_8_i,
  co_clk_i,
  co_tkn_o,
  co_v_i,
  core_async_reset_i,
  ddr_addr_0_o,
  ddr_addr_10_o,
  ddr_addr_11_o,
  ddr_addr_12_o,
  ddr_addr_13_o,
  ddr_addr_14_o,
  ddr_addr_15_o,
  ddr_addr_1_o,
  ddr_addr_2_o,
  ddr_addr_3_o,
  ddr_addr_4_o,
  ddr_addr_5_o,
  ddr_addr_6_o,
  ddr_addr_7_o,
  ddr_addr_8_o,
  ddr_addr_9_o,
  ddr_ba_0_o,
  ddr_ba_1_o,
  ddr_ba_2_o,
  ddr_cas_n_o,
  ddr_ck_n_o,
  ddr_ck_p_o,
  ddr_cke_o,
  ddr_cs_n_o,
  ddr_dm_0_o,
  ddr_dm_1_o,
  ddr_dm_2_o,
  ddr_dm_3_o,
  ddr_dq_0_o,
  ddr_dq_10_o,
  ddr_dq_11_o,
  ddr_dq_12_o,
  ddr_dq_13_o,
  ddr_dq_14_o,
  ddr_dq_15_o,
  ddr_dq_16_o,
  ddr_dq_17_o,
  ddr_dq_18_o,
  ddr_dq_19_o,
  ddr_dq_1_o,
  ddr_dq_20_o,
  ddr_dq_21_o,
  ddr_dq_22_o,
  ddr_dq_23_o,
  ddr_dq_24_o,
  ddr_dq_25_o,
  ddr_dq_26_o,
  ddr_dq_27_o,
  ddr_dq_28_o,
  ddr_dq_29_o,
  ddr_dq_2_o,
  ddr_dq_30_o,
  ddr_dq_31_o,
  ddr_dq_3_o,
  ddr_dq_4_o,
  ddr_dq_5_o,
  ddr_dq_6_o,
  ddr_dq_7_o,
  ddr_dq_8_o,
  ddr_dq_9_o,
  ddr_dqs_n_0_o,
  ddr_dqs_n_1_o,
  ddr_dqs_n_2_o,
  ddr_dqs_n_3_o,
  ddr_dqs_p_0_o,
  ddr_dqs_p_1_o,
  ddr_dqs_p_2_o,
  ddr_dqs_p_3_o,
  ddr_dq_0_i,
  ddr_dq_10_i,
  ddr_dq_11_i,
  ddr_dq_12_i,
  ddr_dq_13_i,
  ddr_dq_14_i,
  ddr_dq_15_i,
  ddr_dq_16_i,
  ddr_dq_17_i,
  ddr_dq_18_i,
  ddr_dq_19_i,
  ddr_dq_1_i,
  ddr_dq_20_i,
  ddr_dq_21_i,
  ddr_dq_22_i,
  ddr_dq_23_i,
  ddr_dq_24_i,
  ddr_dq_25_i,
  ddr_dq_26_i,
  ddr_dq_27_i,
  ddr_dq_28_i,
  ddr_dq_29_i,
  ddr_dq_2_i,
  ddr_dq_30_i,
  ddr_dq_31_i,
  ddr_dq_3_i,
  ddr_dq_4_i,
  ddr_dq_5_i,
  ddr_dq_6_i,
  ddr_dq_7_i,
  ddr_dq_8_i,
  ddr_dq_9_i,
  ddr_dqs_n_0_i,
  ddr_dqs_n_1_i,
  ddr_dqs_n_2_i,
  ddr_dqs_n_3_i,
  ddr_dqs_p_0_i,
  ddr_dqs_p_1_i,
  ddr_dqs_p_2_i,
  ddr_dqs_p_3_i,
  ddr_dq_0_sel,
  ddr_dq_10_sel,
  ddr_dq_11_sel,
  ddr_dq_12_sel,
  ddr_dq_13_sel,
  ddr_dq_14_sel,
  ddr_dq_15_sel,
  ddr_dq_16_sel,
  ddr_dq_17_sel,
  ddr_dq_18_sel,
  ddr_dq_19_sel,
  ddr_dq_1_sel,
  ddr_dq_20_sel,
  ddr_dq_21_sel,
  ddr_dq_22_sel,
  ddr_dq_23_sel,
  ddr_dq_24_sel,
  ddr_dq_25_sel,
  ddr_dq_26_sel,
  ddr_dq_27_sel,
  ddr_dq_28_sel,
  ddr_dq_29_sel,
  ddr_dq_2_sel,
  ddr_dq_30_sel,
  ddr_dq_31_sel,
  ddr_dq_3_sel,
  ddr_dq_4_sel,
  ddr_dq_5_sel,
  ddr_dq_6_sel,
  ddr_dq_7_sel,
  ddr_dq_8_sel,
  ddr_dq_9_sel,
  ddr_dqs_n_0_sel,
  ddr_dqs_n_1_sel,
  ddr_dqs_n_2_sel,
  ddr_dqs_n_3_sel,
  ddr_dqs_p_0_sel,
  ddr_dqs_p_1_sel,
  ddr_dqs_p_2_sel,
  ddr_dqs_p_3_sel,
  ddr_odt_o,
  ddr_ras_n_o,
  ddr_reset_n_o,
  ddr_we_n_o,
  misc_o,
  sel_0_i,
  sel_1_i,
  sel_2_i
) ;

  input bsg_tag_clk_i ;
  output bsg_tag_clk_o ;
  input bsg_tag_data_i ;
  output bsg_tag_data_o ;
  input bsg_tag_en_i ;
  output ci2_0_o ;
  output ci2_1_o ;
  output ci2_2_o ;
  output ci2_3_o ;
  output ci2_4_o ;
  output ci2_5_o ;
  output ci2_6_o ;
  output ci2_7_o ;
  output ci2_8_o ;
  output ci2_clk_o ;
  input ci2_tkn_i ;
  output ci2_v_o ;
  input ci_0_i ;
  input ci_1_i ;
  input ci_2_i ;
  input ci_3_i ;
  input ci_4_i ;
  input ci_5_i ;
  input ci_6_i ;
  input ci_7_i ;
  input ci_8_i ;
  input ci_clk_i ;
  output ci_tkn_o ;
  input ci_v_i ;
  input clk_A_i ;
  input clk_B_i ;
  input clk_C_i ;
  input clk_async_reset_i ;
  output clk_o ;
  output co2_0_o ;
  output co2_1_o ;
  output co2_2_o ;
  output co2_3_o ;
  output co2_4_o ;
  output co2_5_o ;
  output co2_6_o ;
  output co2_7_o ;
  output co2_8_o ;
  output co2_clk_o ;
  input co2_tkn_i ;
  output co2_v_o ;
  input co_0_i ;
  input co_1_i ;
  input co_2_i ;
  input co_3_i ;
  input co_4_i ;
  input co_5_i ;
  input co_6_i ;
  input co_7_i ;
  input co_8_i ;
  input co_clk_i ;
  output co_tkn_o ;
  input co_v_i ;
  input core_async_reset_i ;
  output ddr_addr_0_o ;
  output ddr_addr_10_o ;
  output ddr_addr_11_o ;
  output ddr_addr_12_o ;
  output ddr_addr_13_o ;
  output ddr_addr_14_o ;
  output ddr_addr_15_o ;
  output ddr_addr_1_o ;
  output ddr_addr_2_o ;
  output ddr_addr_3_o ;
  output ddr_addr_4_o ;
  output ddr_addr_5_o ;
  output ddr_addr_6_o ;
  output ddr_addr_7_o ;
  output ddr_addr_8_o ;
  output ddr_addr_9_o ;
  output ddr_ba_0_o ;
  output ddr_ba_1_o ;
  output ddr_ba_2_o ;
  output ddr_cas_n_o ;
  output ddr_ck_n_o ;
  output ddr_ck_p_o ;
  output ddr_cke_o ;
  output ddr_cs_n_o ;
  output ddr_dm_0_o ;
  output ddr_dm_1_o ;
  output ddr_dm_2_o ;
  output ddr_dm_3_o ;
  output ddr_dq_0_o ;
  output ddr_dq_10_o ;
  output ddr_dq_11_o ;
  output ddr_dq_12_o ;
  output ddr_dq_13_o ;
  output ddr_dq_14_o ;
  output ddr_dq_15_o ;
  output ddr_dq_16_o ;
  output ddr_dq_17_o ;
  output ddr_dq_18_o ;
  output ddr_dq_19_o ;
  output ddr_dq_1_o ;
  output ddr_dq_20_o ;
  output ddr_dq_21_o ;
  output ddr_dq_22_o ;
  output ddr_dq_23_o ;
  output ddr_dq_24_o ;
  output ddr_dq_25_o ;
  output ddr_dq_26_o ;
  output ddr_dq_27_o ;
  output ddr_dq_28_o ;
  output ddr_dq_29_o ;
  output ddr_dq_2_o ;
  output ddr_dq_30_o ;
  output ddr_dq_31_o ;
  output ddr_dq_3_o ;
  output ddr_dq_4_o ;
  output ddr_dq_5_o ;
  output ddr_dq_6_o ;
  output ddr_dq_7_o ;
  output ddr_dq_8_o ;
  output ddr_dq_9_o ;
  output ddr_dqs_n_0_o ;
  output ddr_dqs_n_1_o ;
  output ddr_dqs_n_2_o ;
  output ddr_dqs_n_3_o ;
  output ddr_dqs_p_0_o ;
  output ddr_dqs_p_1_o ;
  output ddr_dqs_p_2_o ;
  output ddr_dqs_p_3_o ;
  input ddr_dq_0_i ;
  input ddr_dq_10_i ;
  input ddr_dq_11_i ;
  input ddr_dq_12_i ;
  input ddr_dq_13_i ;
  input ddr_dq_14_i ;
  input ddr_dq_15_i ;
  input ddr_dq_16_i ;
  input ddr_dq_17_i ;
  input ddr_dq_18_i ;
  input ddr_dq_19_i ;
  input ddr_dq_1_i ;
  input ddr_dq_20_i ;
  input ddr_dq_21_i ;
  input ddr_dq_22_i ;
  input ddr_dq_23_i ;
  input ddr_dq_24_i ;
  input ddr_dq_25_i ;
  input ddr_dq_26_i ;
  input ddr_dq_27_i ;
  input ddr_dq_28_i ;
  input ddr_dq_29_i ;
  input ddr_dq_2_i ;
  input ddr_dq_30_i ;
  input ddr_dq_31_i ;
  input ddr_dq_3_i ;
  input ddr_dq_4_i ;
  input ddr_dq_5_i ;
  input ddr_dq_6_i ;
  input ddr_dq_7_i ;
  input ddr_dq_8_i ;
  input ddr_dq_9_i ;
  input ddr_dqs_n_0_i ;
  input ddr_dqs_n_1_i ;
  input ddr_dqs_n_2_i ;
  input ddr_dqs_n_3_i ;
  input ddr_dqs_p_0_i ;
  input ddr_dqs_p_1_i ;
  input ddr_dqs_p_2_i ;
  input ddr_dqs_p_3_i ;
  output ddr_dq_0_sel ;
  output ddr_dq_10_sel ;
  output ddr_dq_11_sel ;
  output ddr_dq_12_sel ;
  output ddr_dq_13_sel ;
  output ddr_dq_14_sel ;
  output ddr_dq_15_sel ;
  output ddr_dq_16_sel ;
  output ddr_dq_17_sel ;
  output ddr_dq_18_sel ;
  output ddr_dq_19_sel ;
  output ddr_dq_1_sel ;
  output ddr_dq_20_sel ;
  output ddr_dq_21_sel ;
  output ddr_dq_22_sel ;
  output ddr_dq_23_sel ;
  output ddr_dq_24_sel ;
  output ddr_dq_25_sel ;
  output ddr_dq_26_sel ;
  output ddr_dq_27_sel ;
  output ddr_dq_28_sel ;
  output ddr_dq_29_sel ;
  output ddr_dq_2_sel ;
  output ddr_dq_30_sel ;
  output ddr_dq_31_sel ;
  output ddr_dq_3_sel ;
  output ddr_dq_4_sel ;
  output ddr_dq_5_sel ;
  output ddr_dq_6_sel ;
  output ddr_dq_7_sel ;
  output ddr_dq_8_sel ;
  output ddr_dq_9_sel ;
  output ddr_dqs_n_0_sel ;
  output ddr_dqs_n_1_sel ;
  output ddr_dqs_n_2_sel ;
  output ddr_dqs_n_3_sel ;
  output ddr_dqs_p_0_sel ;
  output ddr_dqs_p_1_sel ;
  output ddr_dqs_p_2_sel ;
  output ddr_dqs_p_3_sel ;
  output ddr_odt_o ;
  output ddr_ras_n_o ;
  output ddr_reset_n_o ;
  output ddr_we_n_o ;
  output misc_o ;
  input sel_0_i ;
  input sel_1_i ;
  input sel_2_i ;

endmodule


module soc_bsg_black_parrot (
  bsg_tag_clk_i,
  bsg_tag_clk_o,
  bsg_tag_data_i,
  bsg_tag_data_o,
  bsg_tag_en_i,
  ci2_0_o,
  ci2_1_o,
  ci2_2_o,
  ci2_3_o,
  ci2_4_o,
  ci2_5_o,
  ci2_6_o,
  ci2_7_o,
  ci2_8_o,
  ci2_clk_o,
  ci2_tkn_i,
  ci2_v_o,
  ci_0_i,
  ci_1_i,
  ci_2_i,
  ci_3_i,
  ci_4_i,
  ci_5_i,
  ci_6_i,
  ci_7_i,
  ci_8_i,
  ci_clk_i,
  ci_tkn_o,
  ci_v_i,
  clk_A_i,
  clk_B_i,
  clk_C_i,
  clk_async_reset_i,
  clk_o,
  co2_0_o,
  co2_1_o,
  co2_2_o,
  co2_3_o,
  co2_4_o,
  co2_5_o,
  co2_6_o,
  co2_7_o,
  co2_8_o,
  co2_clk_o,
  co2_tkn_i,
  co2_v_o,
  co_0_i,
  co_1_i,
  co_2_i,
  co_3_i,
  co_4_i,
  co_5_i,
  co_6_i,
  co_7_i,
  co_8_i,
  co_clk_i,
  co_tkn_o,
  co_v_i,
  core_async_reset_i,
  ddr_addr_0_o,
  ddr_addr_10_o,
  ddr_addr_11_o,
  ddr_addr_12_o,
  ddr_addr_13_o,
  ddr_addr_14_o,
  ddr_addr_15_o,
  ddr_addr_1_o,
  ddr_addr_2_o,
  ddr_addr_3_o,
  ddr_addr_4_o,
  ddr_addr_5_o,
  ddr_addr_6_o,
  ddr_addr_7_o,
  ddr_addr_8_o,
  ddr_addr_9_o,
  ddr_ba_0_o,
  ddr_ba_1_o,
  ddr_ba_2_o,
  ddr_cas_n_o,
  ddr_ck_n_o,
  ddr_ck_p_o,
  ddr_cke_o,
  ddr_cs_n_o,
  ddr_dm_0_o,
  ddr_dm_1_o,
  ddr_dm_2_o,
  ddr_dm_3_o,
  ddr_dq_0_io,
  ddr_dq_10_io,
  ddr_dq_11_io,
  ddr_dq_12_io,
  ddr_dq_13_io,
  ddr_dq_14_io,
  ddr_dq_15_io,
  ddr_dq_16_io,
  ddr_dq_17_io,
  ddr_dq_18_io,
  ddr_dq_19_io,
  ddr_dq_1_io,
  ddr_dq_20_io,
  ddr_dq_21_io,
  ddr_dq_22_io,
  ddr_dq_23_io,
  ddr_dq_24_io,
  ddr_dq_25_io,
  ddr_dq_26_io,
  ddr_dq_27_io,
  ddr_dq_28_io,
  ddr_dq_29_io,
  ddr_dq_2_io,
  ddr_dq_30_io,
  ddr_dq_31_io,
  ddr_dq_3_io,
  ddr_dq_4_io,
  ddr_dq_5_io,
  ddr_dq_6_io,
  ddr_dq_7_io,
  ddr_dq_8_io,
  ddr_dq_9_io,
  ddr_dqs_n_0_io,
  ddr_dqs_n_1_io,
  ddr_dqs_n_2_io,
  ddr_dqs_n_3_io,
  ddr_dqs_p_0_io,
  ddr_dqs_p_1_io,
  ddr_dqs_p_2_io,
  ddr_dqs_p_3_io,
  ddr_odt_o,
  ddr_ras_n_o,
  ddr_reset_n_o,
  ddr_we_n_o,
  misc_o,
  sel_0_i,
  sel_1_i,
  sel_2_i
) ;

  input bsg_tag_clk_i ;
  output bsg_tag_clk_o ;
  input bsg_tag_data_i ;
  output bsg_tag_data_o ;
  input bsg_tag_en_i ;
  output ci2_0_o ;
  output ci2_1_o ;
  output ci2_2_o ;
  output ci2_3_o ;
  output ci2_4_o ;
  output ci2_5_o ;
  output ci2_6_o ;
  output ci2_7_o ;
  output ci2_8_o ;
  output ci2_clk_o ;
  input ci2_tkn_i ;
  output ci2_v_o ;
  input ci_0_i ;
  input ci_1_i ;
  input ci_2_i ;
  input ci_3_i ;
  input ci_4_i ;
  input ci_5_i ;
  input ci_6_i ;
  input ci_7_i ;
  input ci_8_i ;
  input ci_clk_i ;
  output ci_tkn_o ;
  input ci_v_i ;
  input clk_A_i ;
  input clk_B_i ;
  input clk_C_i ;
  input clk_async_reset_i ;
  output clk_o ;
  output co2_0_o ;
  output co2_1_o ;
  output co2_2_o ;
  output co2_3_o ;
  output co2_4_o ;
  output co2_5_o ;
  output co2_6_o ;
  output co2_7_o ;
  output co2_8_o ;
  output co2_clk_o ;
  input co2_tkn_i ;
  output co2_v_o ;
  input co_0_i ;
  input co_1_i ;
  input co_2_i ;
  input co_3_i ;
  input co_4_i ;
  input co_5_i ;
  input co_6_i ;
  input co_7_i ;
  input co_8_i ;
  input co_clk_i ;
  output co_tkn_o ;
  input co_v_i ;
  input core_async_reset_i ;
  output ddr_addr_0_o ;
  output ddr_addr_10_o ;
  output ddr_addr_11_o ;
  output ddr_addr_12_o ;
  output ddr_addr_13_o ;
  output ddr_addr_14_o ;
  output ddr_addr_15_o ;
  output ddr_addr_1_o ;
  output ddr_addr_2_o ;
  output ddr_addr_3_o ;
  output ddr_addr_4_o ;
  output ddr_addr_5_o ;
  output ddr_addr_6_o ;
  output ddr_addr_7_o ;
  output ddr_addr_8_o ;
  output ddr_addr_9_o ;
  output ddr_ba_0_o ;
  output ddr_ba_1_o ;
  output ddr_ba_2_o ;
  output ddr_cas_n_o ;
  output ddr_ck_n_o ;
  output ddr_ck_p_o ;
  output ddr_cke_o ;
  output ddr_cs_n_o ;
  output ddr_dm_0_o ;
  output ddr_dm_1_o ;
  output ddr_dm_2_o ;
  output ddr_dm_3_o ;
  inout ddr_dq_0_io ;
  inout ddr_dq_10_io ;
  inout ddr_dq_11_io ;
  inout ddr_dq_12_io ;
  inout ddr_dq_13_io ;
  inout ddr_dq_14_io ;
  inout ddr_dq_15_io ;
  inout ddr_dq_16_io ;
  inout ddr_dq_17_io ;
  inout ddr_dq_18_io ;
  inout ddr_dq_19_io ;
  inout ddr_dq_1_io ;
  inout ddr_dq_20_io ;
  inout ddr_dq_21_io ;
  inout ddr_dq_22_io ;
  inout ddr_dq_23_io ;
  inout ddr_dq_24_io ;
  inout ddr_dq_25_io ;
  inout ddr_dq_26_io ;
  inout ddr_dq_27_io ;
  inout ddr_dq_28_io ;
  inout ddr_dq_29_io ;
  inout ddr_dq_2_io ;
  inout ddr_dq_30_io ;
  inout ddr_dq_31_io ;
  inout ddr_dq_3_io ;
  inout ddr_dq_4_io ;
  inout ddr_dq_5_io ;
  inout ddr_dq_6_io ;
  inout ddr_dq_7_io ;
  inout ddr_dq_8_io ;
  inout ddr_dq_9_io ;
  inout ddr_dqs_n_0_io ;
  inout ddr_dqs_n_1_io ;
  inout ddr_dqs_n_2_io ;
  inout ddr_dqs_n_3_io ;
  inout ddr_dqs_p_0_io ;
  inout ddr_dqs_p_1_io ;
  inout ddr_dqs_p_2_io ;
  inout ddr_dqs_p_3_io ;
  output ddr_odt_o ;
  output ddr_ras_n_o ;
  output ddr_reset_n_o ;
  output ddr_we_n_o ;
  output misc_o ;
  input sel_0_i ;
  input sel_1_i ;
  input sel_2_i ;

  bsg_black_parrot u_design (
    .bsg_tag_clk_i(core_bsg_tag_clk_i),
    .bsg_tag_clk_o(core_bsg_tag_clk_o),
    .bsg_tag_data_i(core_bsg_tag_data_i),
    .bsg_tag_data_o(core_bsg_tag_data_o),
    .bsg_tag_en_i(core_bsg_tag_en_i),
    .ci2_0_o(core_ci2_0_o),
    .ci2_1_o(core_ci2_1_o),
    .ci2_2_o(core_ci2_2_o),
    .ci2_3_o(core_ci2_3_o),
    .ci2_4_o(core_ci2_4_o),
    .ci2_5_o(core_ci2_5_o),
    .ci2_6_o(core_ci2_6_o),
    .ci2_7_o(core_ci2_7_o),
    .ci2_8_o(core_ci2_8_o),
    .ci2_clk_o(core_ci2_clk_o),
    .ci2_tkn_i(core_ci2_tkn_i),
    .ci2_v_o(core_ci2_v_o),
    .ci_0_i(core_ci_0_i),
    .ci_1_i(core_ci_1_i),
    .ci_2_i(core_ci_2_i),
    .ci_3_i(core_ci_3_i),
    .ci_4_i(core_ci_4_i),
    .ci_5_i(core_ci_5_i),
    .ci_6_i(core_ci_6_i),
    .ci_7_i(core_ci_7_i),
    .ci_8_i(core_ci_8_i),
    .ci_clk_i(core_ci_clk_i),
    .ci_tkn_o(core_ci_tkn_o),
    .ci_v_i(core_ci_v_i),
    .clk_A_i(core_clk_A_i),
    .clk_B_i(core_clk_B_i),
    .clk_C_i(core_clk_C_i),
    .clk_async_reset_i(core_clk_async_reset_i),
    .clk_o(core_clk_o),
    .co2_0_o(core_co2_0_o),
    .co2_1_o(core_co2_1_o),
    .co2_2_o(core_co2_2_o),
    .co2_3_o(core_co2_3_o),
    .co2_4_o(core_co2_4_o),
    .co2_5_o(core_co2_5_o),
    .co2_6_o(core_co2_6_o),
    .co2_7_o(core_co2_7_o),
    .co2_8_o(core_co2_8_o),
    .co2_clk_o(core_co2_clk_o),
    .co2_tkn_i(core_co2_tkn_i),
    .co2_v_o(core_co2_v_o),
    .co_0_i(core_co_0_i),
    .co_1_i(core_co_1_i),
    .co_2_i(core_co_2_i),
    .co_3_i(core_co_3_i),
    .co_4_i(core_co_4_i),
    .co_5_i(core_co_5_i),
    .co_6_i(core_co_6_i),
    .co_7_i(core_co_7_i),
    .co_8_i(core_co_8_i),
    .co_clk_i(core_co_clk_i),
    .co_tkn_o(core_co_tkn_o),
    .co_v_i(core_co_v_i),
    .core_async_reset_i(core_core_async_reset_i),
    .ddr_addr_0_o(core_ddr_addr_0_o),
    .ddr_addr_10_o(core_ddr_addr_10_o),
    .ddr_addr_11_o(core_ddr_addr_11_o),
    .ddr_addr_12_o(core_ddr_addr_12_o),
    .ddr_addr_13_o(core_ddr_addr_13_o),
    .ddr_addr_14_o(core_ddr_addr_14_o),
    .ddr_addr_15_o(core_ddr_addr_15_o),
    .ddr_addr_1_o(core_ddr_addr_1_o),
    .ddr_addr_2_o(core_ddr_addr_2_o),
    .ddr_addr_3_o(core_ddr_addr_3_o),
    .ddr_addr_4_o(core_ddr_addr_4_o),
    .ddr_addr_5_o(core_ddr_addr_5_o),
    .ddr_addr_6_o(core_ddr_addr_6_o),
    .ddr_addr_7_o(core_ddr_addr_7_o),
    .ddr_addr_8_o(core_ddr_addr_8_o),
    .ddr_addr_9_o(core_ddr_addr_9_o),
    .ddr_ba_0_o(core_ddr_ba_0_o),
    .ddr_ba_1_o(core_ddr_ba_1_o),
    .ddr_ba_2_o(core_ddr_ba_2_o),
    .ddr_cas_n_o(core_ddr_cas_n_o),
    .ddr_ck_n_o(core_ddr_ck_n_o),
    .ddr_ck_p_o(core_ddr_ck_p_o),
    .ddr_cke_o(core_ddr_cke_o),
    .ddr_cs_n_o(core_ddr_cs_n_o),
    .ddr_dm_0_o(core_ddr_dm_0_o),
    .ddr_dm_1_o(core_ddr_dm_1_o),
    .ddr_dm_2_o(core_ddr_dm_2_o),
    .ddr_dm_3_o(core_ddr_dm_3_o),
    .ddr_dq_0_o(core_ddr_dq_0_o),
    .ddr_dq_10_o(core_ddr_dq_10_o),
    .ddr_dq_11_o(core_ddr_dq_11_o),
    .ddr_dq_12_o(core_ddr_dq_12_o),
    .ddr_dq_13_o(core_ddr_dq_13_o),
    .ddr_dq_14_o(core_ddr_dq_14_o),
    .ddr_dq_15_o(core_ddr_dq_15_o),
    .ddr_dq_16_o(core_ddr_dq_16_o),
    .ddr_dq_17_o(core_ddr_dq_17_o),
    .ddr_dq_18_o(core_ddr_dq_18_o),
    .ddr_dq_19_o(core_ddr_dq_19_o),
    .ddr_dq_1_o(core_ddr_dq_1_o),
    .ddr_dq_20_o(core_ddr_dq_20_o),
    .ddr_dq_21_o(core_ddr_dq_21_o),
    .ddr_dq_22_o(core_ddr_dq_22_o),
    .ddr_dq_23_o(core_ddr_dq_23_o),
    .ddr_dq_24_o(core_ddr_dq_24_o),
    .ddr_dq_25_o(core_ddr_dq_25_o),
    .ddr_dq_26_o(core_ddr_dq_26_o),
    .ddr_dq_27_o(core_ddr_dq_27_o),
    .ddr_dq_28_o(core_ddr_dq_28_o),
    .ddr_dq_29_o(core_ddr_dq_29_o),
    .ddr_dq_2_o(core_ddr_dq_2_o),
    .ddr_dq_30_o(core_ddr_dq_30_o),
    .ddr_dq_31_o(core_ddr_dq_31_o),
    .ddr_dq_3_o(core_ddr_dq_3_o),
    .ddr_dq_4_o(core_ddr_dq_4_o),
    .ddr_dq_5_o(core_ddr_dq_5_o),
    .ddr_dq_6_o(core_ddr_dq_6_o),
    .ddr_dq_7_o(core_ddr_dq_7_o),
    .ddr_dq_8_o(core_ddr_dq_8_o),
    .ddr_dq_9_o(core_ddr_dq_9_o),
    .ddr_dqs_n_0_o(core_ddr_dqs_n_0_o),
    .ddr_dqs_n_1_o(core_ddr_dqs_n_1_o),
    .ddr_dqs_n_2_o(core_ddr_dqs_n_2_o),
    .ddr_dqs_n_3_o(core_ddr_dqs_n_3_o),
    .ddr_dqs_p_0_o(core_ddr_dqs_p_0_o),
    .ddr_dqs_p_1_o(core_ddr_dqs_p_1_o),
    .ddr_dqs_p_2_o(core_ddr_dqs_p_2_o),
    .ddr_dqs_p_3_o(core_ddr_dqs_p_3_o),
    .ddr_dq_0_i(core_ddr_dq_0_i),
    .ddr_dq_10_i(core_ddr_dq_10_i),
    .ddr_dq_11_i(core_ddr_dq_11_i),
    .ddr_dq_12_i(core_ddr_dq_12_i),
    .ddr_dq_13_i(core_ddr_dq_13_i),
    .ddr_dq_14_i(core_ddr_dq_14_i),
    .ddr_dq_15_i(core_ddr_dq_15_i),
    .ddr_dq_16_i(core_ddr_dq_16_i),
    .ddr_dq_17_i(core_ddr_dq_17_i),
    .ddr_dq_18_i(core_ddr_dq_18_i),
    .ddr_dq_19_i(core_ddr_dq_19_i),
    .ddr_dq_1_i(core_ddr_dq_1_i),
    .ddr_dq_20_i(core_ddr_dq_20_i),
    .ddr_dq_21_i(core_ddr_dq_21_i),
    .ddr_dq_22_i(core_ddr_dq_22_i),
    .ddr_dq_23_i(core_ddr_dq_23_i),
    .ddr_dq_24_i(core_ddr_dq_24_i),
    .ddr_dq_25_i(core_ddr_dq_25_i),
    .ddr_dq_26_i(core_ddr_dq_26_i),
    .ddr_dq_27_i(core_ddr_dq_27_i),
    .ddr_dq_28_i(core_ddr_dq_28_i),
    .ddr_dq_29_i(core_ddr_dq_29_i),
    .ddr_dq_2_i(core_ddr_dq_2_i),
    .ddr_dq_30_i(core_ddr_dq_30_i),
    .ddr_dq_31_i(core_ddr_dq_31_i),
    .ddr_dq_3_i(core_ddr_dq_3_i),
    .ddr_dq_4_i(core_ddr_dq_4_i),
    .ddr_dq_5_i(core_ddr_dq_5_i),
    .ddr_dq_6_i(core_ddr_dq_6_i),
    .ddr_dq_7_i(core_ddr_dq_7_i),
    .ddr_dq_8_i(core_ddr_dq_8_i),
    .ddr_dq_9_i(core_ddr_dq_9_i),
    .ddr_dqs_n_0_i(core_ddr_dqs_n_0_i),
    .ddr_dqs_n_1_i(core_ddr_dqs_n_1_i),
    .ddr_dqs_n_2_i(core_ddr_dqs_n_2_i),
    .ddr_dqs_n_3_i(core_ddr_dqs_n_3_i),
    .ddr_dqs_p_0_i(core_ddr_dqs_p_0_i),
    .ddr_dqs_p_1_i(core_ddr_dqs_p_1_i),
    .ddr_dqs_p_2_i(core_ddr_dqs_p_2_i),
    .ddr_dqs_p_3_i(core_ddr_dqs_p_3_i),
    .ddr_dq_0_sel(core_ddr_dq_0_sel),
    .ddr_dq_10_sel(core_ddr_dq_10_sel),
    .ddr_dq_11_sel(core_ddr_dq_11_sel),
    .ddr_dq_12_sel(core_ddr_dq_12_sel),
    .ddr_dq_13_sel(core_ddr_dq_13_sel),
    .ddr_dq_14_sel(core_ddr_dq_14_sel),
    .ddr_dq_15_sel(core_ddr_dq_15_sel),
    .ddr_dq_16_sel(core_ddr_dq_16_sel),
    .ddr_dq_17_sel(core_ddr_dq_17_sel),
    .ddr_dq_18_sel(core_ddr_dq_18_sel),
    .ddr_dq_19_sel(core_ddr_dq_19_sel),
    .ddr_dq_1_sel(core_ddr_dq_1_sel),
    .ddr_dq_20_sel(core_ddr_dq_20_sel),
    .ddr_dq_21_sel(core_ddr_dq_21_sel),
    .ddr_dq_22_sel(core_ddr_dq_22_sel),
    .ddr_dq_23_sel(core_ddr_dq_23_sel),
    .ddr_dq_24_sel(core_ddr_dq_24_sel),
    .ddr_dq_25_sel(core_ddr_dq_25_sel),
    .ddr_dq_26_sel(core_ddr_dq_26_sel),
    .ddr_dq_27_sel(core_ddr_dq_27_sel),
    .ddr_dq_28_sel(core_ddr_dq_28_sel),
    .ddr_dq_29_sel(core_ddr_dq_29_sel),
    .ddr_dq_2_sel(core_ddr_dq_2_sel),
    .ddr_dq_30_sel(core_ddr_dq_30_sel),
    .ddr_dq_31_sel(core_ddr_dq_31_sel),
    .ddr_dq_3_sel(core_ddr_dq_3_sel),
    .ddr_dq_4_sel(core_ddr_dq_4_sel),
    .ddr_dq_5_sel(core_ddr_dq_5_sel),
    .ddr_dq_6_sel(core_ddr_dq_6_sel),
    .ddr_dq_7_sel(core_ddr_dq_7_sel),
    .ddr_dq_8_sel(core_ddr_dq_8_sel),
    .ddr_dq_9_sel(core_ddr_dq_9_sel),
    .ddr_dqs_n_0_sel(core_ddr_dqs_n_0_sel),
    .ddr_dqs_n_1_sel(core_ddr_dqs_n_1_sel),
    .ddr_dqs_n_2_sel(core_ddr_dqs_n_2_sel),
    .ddr_dqs_n_3_sel(core_ddr_dqs_n_3_sel),
    .ddr_dqs_p_0_sel(core_ddr_dqs_p_0_sel),
    .ddr_dqs_p_1_sel(core_ddr_dqs_p_1_sel),
    .ddr_dqs_p_2_sel(core_ddr_dqs_p_2_sel),
    .ddr_dqs_p_3_sel(core_ddr_dqs_p_3_sel),
    .ddr_odt_o(core_ddr_odt_o),
    .ddr_ras_n_o(core_ddr_ras_n_o),
    .ddr_reset_n_o(core_ddr_reset_n_o),
    .ddr_we_n_o(core_ddr_we_n_o),
    .misc_o(core_misc_o),
    .sel_0_i(core_sel_0_i),
    .sel_1_i(core_sel_1_i),
    .sel_2_i(core_sel_2_i)
  ) ;

  PADCELL_SIG_V u_ci2_0_o (.PAD(ci2_0_o), .A(core_ci2_0_o)) ; 
  PADCELL_SIG_V u_ci2_1_o (.PAD(ci2_1_o), .A(core_ci2_1_o)) ; 
  PADCELL_SIG_V u_ci2_2_o (.PAD(ci2_2_o), .A(core_ci2_2_o)) ; 
  PADCELL_SIG_V u_ci2_3_o (.PAD(ci2_3_o), .A(core_ci2_3_o)) ; 
  PADCELL_SIG_V u_ci2_4_o (.PAD(ci2_4_o), .A(core_ci2_4_o)) ; 
  PADCELL_SIG_V u_ci2_5_o (.PAD(ci2_5_o), .A(core_ci2_5_o)) ; 
  PADCELL_SIG_V u_ci2_6_o (.PAD(ci2_6_o), .A(core_ci2_6_o)) ; 
  PADCELL_SIG_V u_ci2_7_o (.PAD(ci2_7_o), .A(core_ci2_7_o)) ; 
  PADCELL_SIG_V u_ci2_8_o (.PAD(ci2_8_o), .A(core_ci2_8_o)) ; 
  PADCELL_SIG_V u_ci2_clk_o (.PAD(ci2_clk_o), .A(core_ci2_clk_o)) ; 
  PADCELL_SIG_V u_ci2_tkn_i (.PAD(ci2_tkn_i), .Y(core_ci2_tkn_i)) ; 
  PADCELL_SIG_V u_ci2_v_o (.PAD(ci2_v_o), .A(core_ci2_v_o)) ; 
  PADCELL_SIG_V u_clk_A_i (.PAD(clk_A_i), .Y(core_clk_A_i)) ; 
  PADCELL_SIG_V u_clk_B_i (.PAD(clk_B_i), .Y(core_clk_B_i)) ; 
  PADCELL_SIG_V u_clk_C_i (.PAD(clk_C_i), .Y(core_clk_C_i)) ; 
  PADCELL_SIG_V u_clk_async_reset_i (.PAD(clk_async_reset_i), .Y(core_clk_async_reset_i)) ; 
  PADCELL_SIG_V u_clk_o (.PAD(clk_o), .A(core_clk_o)) ; 
  PADCELL_SIG_V u_co_0_i (.PAD(co_0_i), .Y(core_co_0_i)) ; 
  PADCELL_SIG_V u_co_1_i (.PAD(co_1_i), .Y(core_co_1_i)) ; 
  PADCELL_SIG_V u_co_2_i (.PAD(co_2_i), .Y(core_co_2_i)) ; 
  PADCELL_SIG_V u_co_3_i (.PAD(co_3_i), .Y(core_co_3_i)) ; 
  PADCELL_SIG_V u_co_4_i (.PAD(co_4_i), .Y(core_co_4_i)) ; 
  PADCELL_SIG_V u_co_5_i (.PAD(co_5_i), .Y(core_co_5_i)) ; 
  PADCELL_SIG_V u_co_6_i (.PAD(co_6_i), .Y(core_co_6_i)) ; 
  PADCELL_SIG_V u_co_7_i (.PAD(co_7_i), .Y(core_co_7_i)) ; 
  PADCELL_SIG_V u_co_8_i (.PAD(co_8_i), .Y(core_co_8_i)) ; 
  PADCELL_SIG_V u_co_clk_i (.PAD(co_clk_i), .Y(core_co_clk_i)) ; 
  PADCELL_SIG_V u_co_tkn_o (.PAD(co_tkn_o), .A(core_co_tkn_o)) ; 
  PADCELL_SIG_V u_co_v_i (.PAD(co_v_i), .Y(core_co_v_i)) ; 
  PADCELL_SIG_V u_core_async_reset_i (.PAD(core_async_reset_i), .Y(core_core_async_reset_i)) ; 
  PADCELL_SIG_V u_ddr_addr_0_o (.PAD(ddr_addr_0_o), .A(core_ddr_addr_0_o)) ; 
  PADCELL_SIG_V u_ddr_addr_10_o (.PAD(ddr_addr_10_o), .A(core_ddr_addr_10_o)) ; 
  PADCELL_SIG_V u_ddr_addr_11_o (.PAD(ddr_addr_11_o), .A(core_ddr_addr_11_o)) ; 
  PADCELL_SIG_V u_ddr_addr_12_o (.PAD(ddr_addr_12_o), .A(core_ddr_addr_12_o)) ; 
  PADCELL_SIG_V u_ddr_addr_13_o (.PAD(ddr_addr_13_o), .A(core_ddr_addr_13_o)) ; 
  PADCELL_SIG_V u_ddr_addr_14_o (.PAD(ddr_addr_14_o), .A(core_ddr_addr_14_o)) ; 
  PADCELL_SIG_V u_ddr_addr_15_o (.PAD(ddr_addr_15_o), .A(core_ddr_addr_15_o)) ; 
  PADCELL_SIG_V u_ddr_addr_1_o (.PAD(ddr_addr_1_o), .A(core_ddr_addr_1_o)) ; 
  PADCELL_SIG_V u_ddr_addr_2_o (.PAD(ddr_addr_2_o), .A(core_ddr_addr_2_o)) ; 
  PADCELL_SIG_V u_ddr_addr_3_o (.PAD(ddr_addr_3_o), .A(core_ddr_addr_3_o)) ; 
  PADCELL_SIG_V u_ddr_addr_4_o (.PAD(ddr_addr_4_o), .A(core_ddr_addr_4_o)) ; 
  PADCELL_SIG_V u_ddr_addr_5_o (.PAD(ddr_addr_5_o), .A(core_ddr_addr_5_o)) ; 
  PADCELL_SIG_V u_ddr_addr_6_o (.PAD(ddr_addr_6_o), .A(core_ddr_addr_6_o)) ; 
  PADCELL_SIG_V u_ddr_addr_7_o (.PAD(ddr_addr_7_o), .A(core_ddr_addr_7_o)) ; 
  PADCELL_SIG_V u_ddr_addr_8_o (.PAD(ddr_addr_8_o), .A(core_ddr_addr_8_o)) ; 
  PADCELL_SIG_V u_ddr_addr_9_o (.PAD(ddr_addr_9_o), .A(core_ddr_addr_9_o)) ; 
  PADCELL_SIG_V u_ddr_ba_0_o (.PAD(ddr_ba_0_o), .A(core_ddr_ba_0_o)) ; 
  PADCELL_SIG_V u_ddr_ba_1_o (.PAD(ddr_ba_1_o), .A(core_ddr_ba_1_o)) ; 
  PADCELL_SIG_V u_ddr_ba_2_o (.PAD(ddr_ba_2_o), .A(core_ddr_ba_2_o)) ; 
  PADCELL_SIG_V u_ddr_cas_n_o (.PAD(ddr_cas_n_o), .A(core_ddr_cas_n_o)) ; 
  PADCELL_SIG_V u_ddr_ck_n_o (.PAD(ddr_ck_n_o), .A(core_ddr_ck_n_o)) ; 
  PADCELL_SIG_V u_ddr_ck_p_o (.PAD(ddr_ck_p_o), .A(core_ddr_ck_p_o)) ; 
  PADCELL_SIG_V u_ddr_cke_o (.PAD(ddr_cke_o), .A(core_ddr_cke_o)) ; 
  PADCELL_SIG_V u_ddr_cs_n_o (.PAD(ddr_cs_n_o), .A(core_ddr_cs_n_o)) ; 
  PADCELL_SIG_V u_ddr_dm_1_o (.PAD(ddr_dm_1_o), .A(core_ddr_dm_1_o)) ; 
  PADCELL_SIG_V u_ddr_dm_2_o (.PAD(ddr_dm_2_o), .A(core_ddr_dm_2_o)) ; 
  PADCELL_SIG_V u_ddr_dqs_n_1_io (.PAD(ddr_dqs_n_1_io), .A(core_ddr_dqs_n_1_o), .Y(core_ddr_dqs_n_1_i), .OE(core_ddr_dqs_n_1_sel), .PU(core_ddr_dqs_n_1_sel)) ;
  PADCELL_SIG_V u_ddr_dqs_n_2_io (.PAD(ddr_dqs_n_2_io), .A(core_ddr_dqs_n_2_o), .Y(core_ddr_dqs_n_2_i), .OE(core_ddr_dqs_n_2_sel), .PU(core_ddr_dqs_n_2_sel)) ;
  PADCELL_SIG_V u_ddr_dqs_p_1_io (.PAD(ddr_dqs_p_1_io), .A(core_ddr_dqs_p_1_o), .Y(core_ddr_dqs_p_1_i), .OE(core_ddr_dqs_p_1_sel), .PU(core_ddr_dqs_p_1_sel)) ;
  PADCELL_SIG_V u_ddr_dqs_p_2_io (.PAD(ddr_dqs_p_2_io), .A(core_ddr_dqs_p_2_o), .Y(core_ddr_dqs_p_2_i), .OE(core_ddr_dqs_p_2_sel), .PU(core_ddr_dqs_p_2_sel)) ;
  PADCELL_SIG_V u_ddr_odt_o (.PAD(ddr_odt_o), .A(core_ddr_odt_o)) ; 
  PADCELL_SIG_V u_ddr_ras_n_o (.PAD(ddr_ras_n_o), .A(core_ddr_ras_n_o)) ; 
  PADCELL_SIG_V u_ddr_reset_n_o (.PAD(ddr_reset_n_o), .A(core_ddr_reset_n_o)) ; 
  PADCELL_SIG_V u_ddr_we_n_o (.PAD(ddr_we_n_o), .A(core_ddr_we_n_o)) ; 
  PADCELL_SIG_V u_misc_o (.PAD(misc_o), .A(core_misc_o)) ; 
  PADCELL_SIG_V u_sel_0_i (.PAD(sel_0_i), .Y(core_sel_0_i)) ; 
  PADCELL_SIG_V u_sel_1_i (.PAD(sel_1_i), .Y(core_sel_1_i)) ; 
  PADCELL_SIG_V u_sel_2_i (.PAD(sel_2_i), .Y(core_sel_2_i)) ; 

  PADCELL_SIG_H u_bsg_tag_clk_i (.PAD(bsg_tag_clk_i), .Y(core_bsg_tag_clk_i)) ; 
  PADCELL_SIG_H u_bsg_tag_clk_o (.PAD(bsg_tag_clk_o), .A(core_bsg_tag_clk_o)) ; 
  PADCELL_SIG_H u_bsg_tag_data_i (.PAD(bsg_tag_data_i), .Y(core_bsg_tag_data_i)) ; 
  PADCELL_SIG_H u_bsg_tag_data_o (.PAD(bsg_tag_data_o), .A(core_bsg_tag_data_o)) ; 
  PADCELL_SIG_H u_bsg_tag_en_i (.PAD(bsg_tag_en_i), .Y(core_bsg_tag_en_i)) ; 
  PADCELL_SIG_H u_ci_0_i (.PAD(ci_0_i), .Y(core_ci_0_i)) ; 
  PADCELL_SIG_H u_ci_1_i (.PAD(ci_1_i), .Y(core_ci_1_i)) ; 
  PADCELL_SIG_H u_ci_2_i (.PAD(ci_2_i), .Y(core_ci_2_i)) ; 
  PADCELL_SIG_H u_ci_3_i (.PAD(ci_3_i), .Y(core_ci_3_i)) ; 
  PADCELL_SIG_H u_ci_4_i (.PAD(ci_4_i), .Y(core_ci_4_i)) ; 
  PADCELL_SIG_H u_ci_5_i (.PAD(ci_5_i), .Y(core_ci_5_i)) ; 
  PADCELL_SIG_H u_ci_6_i (.PAD(ci_6_i), .Y(core_ci_6_i)) ; 
  PADCELL_SIG_H u_ci_7_i (.PAD(ci_7_i), .Y(core_ci_7_i)) ; 
  PADCELL_SIG_H u_ci_8_i (.PAD(ci_8_i), .Y(core_ci_8_i)) ; 
  PADCELL_SIG_H u_ci_clk_i (.PAD(ci_clk_i), .Y(core_ci_clk_i)) ; 
  PADCELL_SIG_H u_ci_tkn_o (.PAD(ci_tkn_o), .A(core_ci_tkn_o)) ; 
  PADCELL_SIG_H u_ci_v_i (.PAD(ci_v_i), .Y(core_ci_v_i)) ; 
  PADCELL_SIG_H u_co2_0_o (.PAD(co2_0_o), .A(core_co2_0_o)) ; 
  PADCELL_SIG_H u_co2_1_o (.PAD(co2_1_o), .A(core_co2_1_o)) ; 
  PADCELL_SIG_H u_co2_2_o (.PAD(co2_2_o), .A(core_co2_2_o)) ; 
  PADCELL_SIG_H u_co2_3_o (.PAD(co2_3_o), .A(core_co2_3_o)) ; 
  PADCELL_SIG_H u_co2_4_o (.PAD(co2_4_o), .A(core_co2_4_o)) ; 
  PADCELL_SIG_H u_co2_5_o (.PAD(co2_5_o), .A(core_co2_5_o)) ; 
  PADCELL_SIG_H u_co2_6_o (.PAD(co2_6_o), .A(core_co2_6_o)) ; 
  PADCELL_SIG_H u_co2_7_o (.PAD(co2_7_o), .A(core_co2_7_o)) ; 
  PADCELL_SIG_H u_co2_8_o (.PAD(co2_8_o), .A(core_co2_8_o)) ; 
  PADCELL_SIG_H u_co2_clk_o (.PAD(co2_clk_o), .A(core_co2_clk_o)) ; 
  PADCELL_SIG_H u_co2_tkn_i (.PAD(co2_tkn_i), .Y(core_co2_tkn_i)) ; 
  PADCELL_SIG_H u_co2_v_o (.PAD(co2_v_o), .A(core_co2_v_o)) ; 
  PADCELL_SIG_H u_ddr_dm_0_o (.PAD(ddr_dm_0_o), .A(core_ddr_dm_0_o)) ; 
  PADCELL_SIG_H u_ddr_dm_3_o (.PAD(ddr_dm_3_o), .A(core_ddr_dm_3_o)) ; 
  PADCELL_SIG_H u_ddr_dq_0_io    (.PAD(ddr_dq_0_io),    .A(core_ddr_dq_0_o),    .Y(core_ddr_dq_0_i),    .OE(core_ddr_dq_0_sel),    .PU(core_ddr_dq_0_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_10_io   (.PAD(ddr_dq_10_io),   .A(core_ddr_dq_10_o),   .Y(core_ddr_dq_10_i),   .OE(core_ddr_dq_10_sel),   .PU(core_ddr_dq_10_sel)) ;
  PADCELL_SIG_H u_ddr_dq_11_io   (.PAD(ddr_dq_11_io),   .A(core_ddr_dq_11_o),   .Y(core_ddr_dq_11_i),   .OE(core_ddr_dq_11_sel),   .PU(core_ddr_dq_11_sel)) ;
  PADCELL_SIG_H u_ddr_dq_12_io   (.PAD(ddr_dq_12_io),   .A(core_ddr_dq_12_o),   .Y(core_ddr_dq_12_i),   .OE(core_ddr_dq_12_sel),   .PU(core_ddr_dq_12_sel)) ;
  PADCELL_SIG_H u_ddr_dq_13_io   (.PAD(ddr_dq_13_io),   .A(core_ddr_dq_13_o),   .Y(core_ddr_dq_13_i),   .OE(core_ddr_dq_13_sel),   .PU(core_ddr_dq_13_sel)) ;
  PADCELL_SIG_H u_ddr_dq_14_io   (.PAD(ddr_dq_14_io),   .A(core_ddr_dq_14_o),   .Y(core_ddr_dq_14_i),   .OE(core_ddr_dq_14_sel),   .PU(core_ddr_dq_14_sel)) ;
  PADCELL_SIG_H u_ddr_dq_15_io   (.PAD(ddr_dq_15_io),   .A(core_ddr_dq_15_o),   .Y(core_ddr_dq_15_i),   .OE(core_ddr_dq_15_sel),   .PU(core_ddr_dq_15_sel)) ;
  PADCELL_SIG_H u_ddr_dq_16_io   (.PAD(ddr_dq_16_io),   .A(core_ddr_dq_16_o),   .Y(core_ddr_dq_16_i),   .OE(core_ddr_dq_16_sel),   .PU(core_ddr_dq_16_sel)) ;
  PADCELL_SIG_H u_ddr_dq_17_io   (.PAD(ddr_dq_17_io),   .A(core_ddr_dq_17_o),   .Y(core_ddr_dq_17_i),   .OE(core_ddr_dq_17_sel),   .PU(core_ddr_dq_17_sel)) ;
  PADCELL_SIG_H u_ddr_dq_18_io   (.PAD(ddr_dq_18_io),   .A(core_ddr_dq_18_o),   .Y(core_ddr_dq_18_i),   .OE(core_ddr_dq_18_sel),   .PU(core_ddr_dq_18_sel)) ;
  PADCELL_SIG_H u_ddr_dq_19_io   (.PAD(ddr_dq_19_io),   .A(core_ddr_dq_19_o),   .Y(core_ddr_dq_19_i),   .OE(core_ddr_dq_19_sel),   .PU(core_ddr_dq_19_sel)) ;
  PADCELL_SIG_H u_ddr_dq_1_io    (.PAD(ddr_dq_1_io),    .A(core_ddr_dq_1_o),    .Y(core_ddr_dq_1_i),    .OE(core_ddr_dq_1_sel),    .PU(core_ddr_dq_1_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_20_io   (.PAD(ddr_dq_20_io),   .A(core_ddr_dq_20_o),   .Y(core_ddr_dq_20_i),   .OE(core_ddr_dq_20_sel),   .PU(core_ddr_dq_20_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_21_io   (.PAD(ddr_dq_21_io),   .A(core_ddr_dq_21_o),   .Y(core_ddr_dq_21_i),   .OE(core_ddr_dq_21_sel),   .PU(core_ddr_dq_21_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_22_io   (.PAD(ddr_dq_22_io),   .A(core_ddr_dq_22_o),   .Y(core_ddr_dq_22_i),   .OE(core_ddr_dq_22_sel),   .PU(core_ddr_dq_22_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_23_io   (.PAD(ddr_dq_23_io),   .A(core_ddr_dq_23_o),   .Y(core_ddr_dq_23_i),   .OE(core_ddr_dq_23_sel),   .PU(core_ddr_dq_23_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_24_io   (.PAD(ddr_dq_24_io),   .A(core_ddr_dq_24_o),   .Y(core_ddr_dq_24_i),   .OE(core_ddr_dq_24_sel),   .PU(core_ddr_dq_24_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_25_io   (.PAD(ddr_dq_25_io),   .A(core_ddr_dq_25_o),   .Y(core_ddr_dq_25_i),   .OE(core_ddr_dq_25_sel),   .PU(core_ddr_dq_25_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_26_io   (.PAD(ddr_dq_26_io),   .A(core_ddr_dq_26_o),   .Y(core_ddr_dq_26_i),   .OE(core_ddr_dq_26_sel),   .PU(core_ddr_dq_26_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_27_io   (.PAD(ddr_dq_27_io),   .A(core_ddr_dq_27_o),   .Y(core_ddr_dq_27_i),   .OE(core_ddr_dq_27_sel),   .PU(core_ddr_dq_27_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_28_io   (.PAD(ddr_dq_28_io),   .A(core_ddr_dq_28_o),   .Y(core_ddr_dq_28_i),   .OE(core_ddr_dq_28_sel),   .PU(core_ddr_dq_28_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_29_io   (.PAD(ddr_dq_29_io),   .A(core_ddr_dq_29_o),   .Y(core_ddr_dq_29_i),   .OE(core_ddr_dq_29_sel),   .PU(core_ddr_dq_29_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_2_io    (.PAD(ddr_dq_2_io),    .A(core_ddr_dq_2_o),    .Y(core_ddr_dq_2_i),    .OE(core_ddr_dq_2_sel),    .PU(core_ddr_dq_2_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_30_io   (.PAD(ddr_dq_30_io),   .A(core_ddr_dq_30_o),   .Y(core_ddr_dq_30_i),   .OE(core_ddr_dq_30_sel),   .PU(core_ddr_dq_30_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_31_io   (.PAD(ddr_dq_31_io),   .A(core_ddr_dq_31_o),   .Y(core_ddr_dq_31_i),   .OE(core_ddr_dq_31_sel),   .PU(core_ddr_dq_31_sel)) ; 
  PADCELL_SIG_H u_ddr_dq_3_io    (.PAD(ddr_dq_3_io),    .A(core_ddr_dq_3_o),    .Y(core_ddr_dq_3_i),    .OE(core_ddr_dq_3_sel),    .PU(core_ddr_dq_3_sel)) ;
  PADCELL_SIG_H u_ddr_dq_4_io    (.PAD(ddr_dq_4_io),    .A(core_ddr_dq_4_o),    .Y(core_ddr_dq_4_i),    .OE(core_ddr_dq_4_sel),    .PU(core_ddr_dq_4_sel)) ;
  PADCELL_SIG_H u_ddr_dq_5_io    (.PAD(ddr_dq_5_io),    .A(core_ddr_dq_5_o),    .Y(core_ddr_dq_5_i),    .OE(core_ddr_dq_5_sel),    .PU(core_ddr_dq_5_sel)) ;
  PADCELL_SIG_H u_ddr_dq_6_io    (.PAD(ddr_dq_6_io),    .A(core_ddr_dq_6_o),    .Y(core_ddr_dq_6_i),    .OE(core_ddr_dq_6_sel),    .PU(core_ddr_dq_6_sel)) ;
  PADCELL_SIG_H u_ddr_dq_7_io    (.PAD(ddr_dq_7_io),    .A(core_ddr_dq_7_o),    .Y(core_ddr_dq_7_i),    .OE(core_ddr_dq_7_sel),    .PU(core_ddr_dq_7_sel)) ;
  PADCELL_SIG_H u_ddr_dq_8_io    (.PAD(ddr_dq_8_io),    .A(core_ddr_dq_8_o),    .Y(core_ddr_dq_8_i),    .OE(core_ddr_dq_8_sel),    .PU(core_ddr_dq_8_sel)) ;
  PADCELL_SIG_H u_ddr_dq_9_io    (.PAD(ddr_dq_9_io),    .A(core_ddr_dq_9_o),    .Y(core_ddr_dq_9_i),    .OE(core_ddr_dq_9_sel),    .PU(core_ddr_dq_9_sel)) ;
  PADCELL_SIG_H u_ddr_dqs_n_0_io (.PAD(ddr_dqs_n_0_io), .A(core_ddr_dqs_n_0_o), .Y(core_ddr_dqs_n_0_i), .OE(core_ddr_dqs_n_0_sel), .PU(core_ddr_dqs_n_0_sel)) ; 
  PADCELL_SIG_H u_ddr_dqs_n_3_io (.PAD(ddr_dqs_n_3_io), .A(core_ddr_dqs_n_3_o), .Y(core_ddr_dqs_n_3_i), .OE(core_ddr_dqs_n_3_sel), .PU(core_ddr_dqs_n_3_sel)) ; 
  PADCELL_SIG_H u_ddr_dqs_p_0_io (.PAD(ddr_dqs_p_0_io), .A(core_ddr_dqs_p_0_o), .Y(core_ddr_dqs_p_0_i), .OE(core_ddr_dqs_p_0_sel), .PU(core_ddr_dqs_p_0_sel)) ; 
  PADCELL_SIG_H u_ddr_dqs_p_3_io (.PAD(ddr_dqs_p_3_io), .A(core_ddr_dqs_p_3_o), .Y(core_ddr_dqs_p_3_i), .OE(core_ddr_dqs_p_3_sel), .PU(core_ddr_dqs_p_3_sel)) ; 

  PADCELL_VDDIO_V u_v18_0 ();
  PADCELL_VDDIO_V u_v18_1 ();
  PADCELL_VDDIO_V u_v18_2 ();
  PADCELL_VDDIO_V u_v18_3 ();
  PADCELL_VDDIO_V u_v18_4 ();
  PADCELL_VDDIO_V u_v18_5 ();
  PADCELL_VDDIO_V u_v18_6 ();
  PADCELL_VDDIO_V u_v18_7 ();
  PADCELL_VDDIO_V u_v18_8 ();
  PADCELL_VDDIO_V u_v18_17 ();
  PADCELL_VDDIO_V u_v18_18 ();
  PADCELL_VDDIO_V u_v18_19 ();
  PADCELL_VDDIO_V u_v18_20 ();
  PADCELL_VDDIO_V u_v18_21 ();
  PADCELL_VDDIO_V u_v18_22 ();
  PADCELL_VDDIO_V u_v18_23 ();
  PADCELL_VDDIO_V u_v18_24 ();
  PADCELL_VDD_V u_vdd_0 ();
  PADCELL_VDD_V u_vdd_1 ();
  PADCELL_VDD_V u_vdd_2 ();
  PADCELL_VDD_V u_vdd_3 ();
  PADCELL_VDD_V u_vdd_4 ();
  PADCELL_VDD_V u_vdd_5 ();
  PADCELL_VDD_V u_vdd_6 ();
  PADCELL_VDD_V u_vdd_7 ();
  PADCELL_VDD_V u_vdd_pll ();
  PADCELL_VDD_V u_vdd_17 ();
  PADCELL_VDD_V u_vdd_18 ();
  PADCELL_VDD_V u_vdd_19 ();
  PADCELL_VDD_V u_vdd_20 ();
  PADCELL_VDD_V u_vdd_21 ();
  PADCELL_VDD_V u_vdd_22 ();
  PADCELL_VDD_V u_vdd_23 ();
  PADCELL_VDD_V u_vdd_24 ();
  PADCELL_VSS_V u_vss_0 ();
  PADCELL_VSS_V u_vss_1 ();
  PADCELL_VSS_V u_vss_2 ();
  PADCELL_VSS_V u_vss_3 ();
  PADCELL_VSS_V u_vss_4 ();
  PADCELL_VSS_V u_vss_5 ();
  PADCELL_VSS_V u_vss_6 ();
  PADCELL_VSS_V u_vss_7 ();
  PADCELL_VSS_V u_vss_pll ();
  PADCELL_VSS_V u_vss_17 ();
  PADCELL_VSS_V u_vss_18 ();
  PADCELL_VSS_V u_vss_19 ();
  PADCELL_VSS_V u_vss_20 ();
  PADCELL_VSS_V u_vss_21 ();
  PADCELL_VSS_V u_vss_22 ();
  PADCELL_VSS_V u_vss_23 ();
  PADCELL_VSS_V u_vss_24 ();
  PADCELL_VSSIO_V u_vzz_0 ();
  PADCELL_VSSIO_V u_vzz_1 ();
  PADCELL_VSSIO_V u_vzz_2 ();
  PADCELL_VSSIO_V u_vzz_3 ();
  PADCELL_VSSIO_V u_vzz_4 ();
  PADCELL_VSSIO_V u_vzz_5 ();
  PADCELL_VSSIO_V u_vzz_6 ();
  PADCELL_VSSIO_V u_vzz_7 ();
  PADCELL_VSSIO_V u_vzz_8 ();
  PADCELL_VSSIO_V u_vzz_17 ();
  PADCELL_VSSIO_V u_vzz_18 ();
  PADCELL_VSSIO_V u_vzz_19 ();
  PADCELL_VSSIO_V u_vzz_20 ();
  PADCELL_VSSIO_V u_vzz_21 ();
  PADCELL_VSSIO_V u_vzz_22 ();
  PADCELL_VSSIO_V u_vzz_23 ();
  PADCELL_VSSIO_V u_vzz_24 ();

  PADCELL_VDDIO_H u_v18_25 ();
  PADCELL_VDDIO_H u_v18_26 ();
  PADCELL_VDDIO_H u_v18_27 ();
  PADCELL_VDDIO_H u_v18_28 ();
  PADCELL_VDDIO_H u_v18_29 ();
  PADCELL_VDDIO_H u_v18_30 ();
  PADCELL_VDDIO_H u_v18_31 ();
  PADCELL_VDDIO_H u_v18_32 ();
  PADCELL_VDDIO_H u_v18_9 ();
  PADCELL_VDDIO_H u_v18_10 ();
  PADCELL_VDDIO_H u_v18_11 ();
  PADCELL_VDDIO_H u_v18_12 ();
  PADCELL_VDDIO_H u_v18_13 ();
  PADCELL_VDDIO_H u_v18_14 ();
  PADCELL_VDDIO_H u_v18_15 ();
  PADCELL_VDDIO_H u_v18_16 ();
  PADCELL_VDD_H u_vdd_25 ();                    
  PADCELL_VDD_H u_vdd_26 ();                    
  PADCELL_VDD_H u_vdd_27 ();
  PADCELL_VDD_H u_vdd_28 ();
  PADCELL_VDD_H u_vdd_29 ();
  PADCELL_VDD_H u_vdd_30 ();
  PADCELL_VDD_H u_vdd_31 ();
  PADCELL_VDD_H u_vdd_32 ();
  PADCELL_VDD_H u_vdd_8 ();
  PADCELL_VDD_H u_vdd_9 ();
  PADCELL_VDD_H u_vdd_10 ();
  PADCELL_VDD_H u_vdd_11 ();
  PADCELL_VDD_H u_vdd_12 ();
  PADCELL_VDD_H u_vdd_13 ();
  PADCELL_VDD_H u_vdd_14 ();
  PADCELL_VDD_H u_vdd_15 ();
  PADCELL_VDD_H u_vdd_16 ();
  PADCELL_VDD_H u_vss_25 ();
  PADCELL_VDD_H u_vss_26 ();
  PADCELL_VDD_H u_vss_27 ();
  PADCELL_VDD_H u_vss_28 ();
  PADCELL_VDD_H u_vss_29 ();
  PADCELL_VDD_H u_vss_30 ();
  PADCELL_VDD_H u_vss_31 ();
  PADCELL_VDD_H u_vss_32 ();
  PADCELL_VDD_H u_vss_8 ();
  PADCELL_VDD_H u_vss_9 ();
  PADCELL_VDD_H u_vss_10 ();
  PADCELL_VDD_H u_vss_11 ();
  PADCELL_VDD_H u_vss_12 ();
  PADCELL_VDD_H u_vss_13 ();
  PADCELL_VDD_H u_vss_14 ();
  PADCELL_VDD_H u_vss_15 ();
  PADCELL_VDD_H u_vss_16 ();
  PADCELL_VSSIO_H u_vzz_25 ();
  PADCELL_VSSIO_H u_vzz_26 ();
  PADCELL_VSSIO_H u_vzz_27 ();
  PADCELL_VSSIO_H u_vzz_28 ();
  PADCELL_VSSIO_H u_vzz_29 ();
  PADCELL_VSSIO_H u_vzz_30 ();
  PADCELL_VSSIO_H u_vzz_31 ();
  PADCELL_VSSIO_H u_vzz_32 ();
  PADCELL_VSSIO_H u_vzz_9 ();
  PADCELL_VSSIO_H u_vzz_10 ();
  PADCELL_VSSIO_H u_vzz_11 ();
  PADCELL_VSSIO_H u_vzz_12 ();
  PADCELL_VSSIO_H u_vzz_13 ();
  PADCELL_VSSIO_H u_vzz_14 ();
  PADCELL_VSSIO_H u_vzz_15 ();
  PADCELL_VSSIO_H u_vzz_16 ();


endmodule 
