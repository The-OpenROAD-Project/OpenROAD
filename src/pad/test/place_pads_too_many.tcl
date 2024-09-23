# Test for placing pads using place_pads with uniform spacing
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

catch {place_pads -row IO_EAST \
    u_ddr_dq_23_io u_ddr_dq_22_io u_ddr_dq_21_io u_vdd_4 u_vss_4 u_ddr_dq_20_io u_vzz_9 \
    u_v18_9 u_ddr_dq_19_io u_ddr_dq_18_io u_ddr_dq_17_io u_ddr_dq_16_io u_vzz_10 u_v18_10 \
    u_ddr_dq_31_io u_ddr_dq_30_io u_ddr_dq_29_io u_ddr_dq_28_io u_vzz_11 u_v18_11 u_ddr_dq_27_io \
    u_vdd_5 u_vss_5 u_ddr_dq_26_io u_ddr_dq_25_io u_ddr_dq_24_io u_vzz_12 u_v18_12 u_ddr_dqs_n_3_io \
    u_ddr_dqs_p_3_io u_ddr_dm_3_o u_bsg_tag_clk_i u_vzz_13 u_v18_13 u_bsg_tag_data_i u_bsg_tag_en_i \
    u_ci_8_i u_vdd_6 u_vss_6 u_ci_7_i u_vzz_14 u_v18_14 u_ci_6_i u_ci_5_i u_ci_v_i u_ci_tkn_o u_vzz_15 \
    u_v18_15 u_ci_clk_i u_ci_4_i u_ci_3_i u_ci_2_i u_vzz_16 u_v18_16 u_ci_1_i u_vdd_7 u_vss_7 u_ci_0_i \
    u_ci2_8_o u_ci2_7_o \
    u_vss_15 u_vdd_15 u_ddr_dq_8_io u_ddr_dq_9_io u_ddr_dq_10_io u_ddr_dq_11_io \
    u_ddr_dq_12_io u_v18_32 u_vzz_32 u_ddr_dq_13_io u_ddr_dq_14_io u_ddr_dq_15_io u_ddr_dqs_p_0_io \
    u_v18_31 u_vzz_31 u_ddr_dqs_n_0_io u_ddr_dm_0_o u_vss_14 u_vdd_14 u_ddr_dq_0_io u_ddr_dq_1_io \
    u_v18_30 u_vzz_30 u_ddr_dq_2_io u_ddr_dq_3_io u_ddr_dq_4_io u_ddr_dq_5_io u_v18_29 u_vzz_29 \
    u_ddr_dq_6_io u_ddr_dq_7_io u_bsg_tag_data_o u_vss_13 u_vdd_13 u_bsg_tag_clk_o u_v18_28 u_vzz_28 \
    u_co2_0_o u_co2_1_o u_co2_2_o u_co2_3_o u_v18_27 u_vzz_27 u_co2_4_o u_co2_clk_o u_co2_tkn_i \
    u_co2_v_o u_v18_26 u_vzz_26 u_vss_12 u_vdd_12 u_co2_5_o u_co2_6_o u_co2_7_o u_co2_8_o} err
puts $err
