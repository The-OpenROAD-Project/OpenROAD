# Test for assigning bumps
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

add_global_connect -pin_pattern "VDD" -net VDD -power
add_global_connect -pin_pattern "DVDD" -net DVDD -power
add_global_connect -pin_pattern "VSS" -net VSS -ground
add_global_connect -pin_pattern "DVSS" -net DVSS -ground

# Make IO Sites
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 35

######## Place Pads ########
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 205.0 u_ddr_dm_1_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 335.0 u_ddr_dqs_n_1_io
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 365.0 u_vzz_0
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 495.0 u_v18_0
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 525.0 u_ddr_dqs_p_1_io
place_pad -master PADCELL_VDD_V -row IO_SOUTH -location 555.0 u_vdd_0
place_pad -master PADCELL_VSS_V -row IO_SOUTH -location 625.0 u_vss_0
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 655.0 u_ddr_ba_2_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 690.0 u_ddr_ba_1_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 720.0 u_ddr_ba_0_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 785.0 u_vzz_1
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 815.0 u_v18_1
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 845.0 u_ddr_addr_15_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 875.0 u_ddr_addr_14_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 910.0 u_ddr_addr_13_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 945.0 u_ddr_addr_12_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 975.0 u_vzz_2
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 1005.0 u_v18_2
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1035.0 u_ddr_addr_11_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1065.0 u_ddr_addr_10_o
place_pad -master PADCELL_VDD_V -row IO_SOUTH -location 1105.0 u_vdd_1
place_pad -master PADCELL_VSS_V -row IO_SOUTH -location 1135.0 u_vss_1
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1170.0 u_ddr_addr_9_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1200.0 u_ddr_addr_8_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 1230.0 u_vzz_3
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 1265.0 u_v18_3
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1295.0 u_ddr_addr_7_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1325.0 u_ddr_addr_6_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1360.0 u_ddr_addr_5_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1390.0 u_ddr_addr_4_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 1425.0 u_vzz_4
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 1455.0 u_v18_4
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1485.0 u_ddr_addr_3_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1515.0 u_ddr_addr_2_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1550.0 u_ddr_addr_1_o
place_pad -master PADCELL_VDD_V -row IO_SOUTH -location 1585.0 u_vdd_2
place_pad -master PADCELL_VSS_V -row IO_SOUTH -location 1615.0 u_vss_2
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1645.0 u_ddr_addr_0_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 1675.0 u_vzz_5
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 1705.0 u_v18_5
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1745.0 u_ddr_odt_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1775.0 u_ddr_reset_n_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1810.0 u_ddr_we_n_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1840.0 u_ddr_cas_n_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 1870.0 u_vzz_6
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 1905.0 u_v18_6
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1935.0 u_ddr_ras_n_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 1965.0 u_ddr_cs_n_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 2000.0 u_ddr_cke_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 2030.0 u_ddr_ck_n_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 2065.0 u_vzz_7
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 2095.0 u_v18_7
place_pad -master PADCELL_VDD_V -row IO_SOUTH -location 2125.0 u_vdd_3
place_pad -master PADCELL_VSS_V -row IO_SOUTH -location 2155.0 u_vss_3
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 2255.0 u_ddr_ck_p_o
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 2285.0 u_ddr_dqs_n_2_io
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 2320.0 u_ddr_dqs_p_2_io
place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 2415.0 u_ddr_dm_2_o
place_pad -master PADCELL_VSSIO_V -row IO_SOUTH -location 2445.0 u_vzz_8
place_pad -master PADCELL_VDDIO_V -row IO_SOUTH -location 2605.0 u_v18_8
place_pad -master PADCELL_SIG_H -row IO_EAST -location 205.0 u_ddr_dq_23_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 335.0 u_ddr_dq_22_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 370.0 u_ddr_dq_21_io
place_pad -master PADCELL_VDD_V -row IO_EAST -location 495.0 u_vdd_4
place_pad -master PADCELL_VSS_V -row IO_EAST -location 525.0 u_vss_4
place_pad -master PADCELL_SIG_H -row IO_EAST -location 555.0 u_ddr_dq_20_io
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 625.0 u_vzz_9
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 655.0 u_v18_9
place_pad -master PADCELL_SIG_H -row IO_EAST -location 685.0 u_ddr_dq_19_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 715.0 u_ddr_dq_18_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 785.0 u_ddr_dq_17_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 815.0 u_ddr_dq_16_io
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 845.0 u_vzz_10
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 875.0 u_v18_10
place_pad -master PADCELL_SIG_H -row IO_EAST -location 905.0 u_ddr_dq_31_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 945.0 u_ddr_dq_30_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 980.0 u_ddr_dq_29_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1010.0 u_ddr_dq_28_io
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 1040.0 u_vzz_11
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 1070.0 u_v18_11
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1105.0 u_ddr_dq_27_io
place_pad -master PADCELL_VDD_V -row IO_EAST -location 1135.0 u_vdd_5
place_pad -master PADCELL_VSS_V -row IO_EAST -location 1165.0 u_vss_5
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1195.0 u_ddr_dq_26_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1230.0 u_ddr_dq_25_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1265.0 u_ddr_dq_24_io
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 1295.0 u_vzz_12
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 1325.0 u_v18_12
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1355.0 u_ddr_dqs_n_3_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1385.0 u_ddr_dqs_p_3_io
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1425.0 u_ddr_dm_3_o
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1455.0 u_bsg_tag_clk_i
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 1485.0 u_vzz_13
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 1515.0 u_v18_13
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1545.0 u_bsg_tag_data_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1585.0 u_bsg_tag_en_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1620.0 u_ci_8_i
place_pad -master PADCELL_VDD_V -row IO_EAST -location 1650.0 u_vdd_6
place_pad -master PADCELL_VSS_V -row IO_EAST -location 1680.0 u_vss_6
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1710.0 u_ci_7_i
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 1745.0 u_vzz_14
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 1775.0 u_v18_14
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1805.0 u_ci_6_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1835.0 u_ci_5_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1870.0 u_ci_v_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1905.0 u_ci_tkn_o
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 1935.0 u_vzz_15
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 1965.0 u_v18_15
place_pad -master PADCELL_SIG_H -row IO_EAST -location 1995.0 u_ci_clk_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 2025.0 u_ci_4_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 2065.0 u_ci_3_i
place_pad -master PADCELL_SIG_H -row IO_EAST -location 2095.0 u_ci_2_i
place_pad -master PADCELL_VSSIO_H -row IO_EAST -location 2125.0 u_vzz_16
place_pad -master PADCELL_VDDIO_H -row IO_EAST -location 2155.0 u_v18_16
place_pad -master PADCELL_SIG_H -row IO_EAST -location 2255.0 u_ci_1_i
place_pad -master PADCELL_VDD_V -row IO_EAST -location 2285.0 u_vdd_7
place_pad -master PADCELL_VSS_V -row IO_EAST -location 2315.0 u_vss_7
place_pad -master PADCELL_SIG_H -row IO_EAST -location 2415.0 u_ci_0_i
place_pad -master PADCELL_SIG_V -row IO_EAST -location 2450.0 u_ci2_8_o
place_pad -master PADCELL_SIG_V -row IO_EAST -location 2605.0 u_ci2_7_o
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 2795.0 u_vzz_17
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 2665.0 u_v18_17
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2635.0 u_ci2_6_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2505.0 u_ci2_5_o
place_pad -master PADCELL_VDD_H -row IO_NORTH -location 2475.0 u_vdd_8
place_pad -master PADCELL_VSS_H -row IO_NORTH -location 2445.0 u_vss_8
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2375.0 u_ci2_v_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2345.0 u_ci2_tkn_i
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 2315.0 u_vzz_18
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 2285.0 u_v18_18
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2220.0 u_ci2_clk_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2190.0 u_ci2_4_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2135.0 u_ci2_3_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2110.0 u_ci2_2_o
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 2085.0 u_vzz_19
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 2060.0 u_v18_19
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2030.0 u_ci2_1_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 2000.0 u_ci2_0_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1965.0 u_core_async_reset_i
place_pad -master PADCELL_VDD_H -row IO_NORTH -location 1935.0 u_vdd_9
place_pad -master PADCELL_VSS_H -row IO_NORTH -location 1900.0 u_vss_9
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1870.0 u_sel_2_i
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 1835.0 u_v18_20
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 1805.0 u_vzz_20
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1775.0 u_sel_1_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1740.0 u_sel_0_i
place_pad -master PADCELL_FBRK_V -row IO_NORTH -location 1735.0 u_brk0
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1705.0 u_misc_o
place_pad -master PADCELL_VSS_V -row IO_NORTH -location 1680.0 u_vss_pll
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1655.0 u_clk_async_reset_i
place_pad -master PADCELL_VDD_V -row IO_NORTH -location 1630.0 u_vdd_pll
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1580.0 u_clk_o
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 1550.0 u_vzz_21
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1520.0 u_clk_C_i
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 1490.0 u_v18_21
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1455.0 u_clk_B_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1415.0 u_clk_A_i
place_pad -master PADCELL_VDD_H -row IO_NORTH -location 1385.0 u_vdd_10
place_pad -master PADCELL_VSS_H -row IO_NORTH -location 1355.0 u_vss_10
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 1325.0 u_vzz_22
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 1295.0 u_v18_22
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1260.0 u_co_8_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1230.0 u_co_7_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1195.0 u_co_6_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1165.0 u_co_5_i
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 1135.0 u_vzz_23
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 1100.0 u_v18_23
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1070.0 u_co_v_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1040.0 u_co_tkn_o
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 1005.0 u_co_clk_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 975.0 u_co_4_i
place_pad -master PADCELL_VSSIO_V -row IO_NORTH -location 935.0 u_vzz_24
place_pad -master PADCELL_VDDIO_V -row IO_NORTH -location 905.0 u_v18_24
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 875.0 u_co_3_i
place_pad -master PADCELL_VDD_H -row IO_NORTH -location 845.0 u_vdd_11
place_pad -master PADCELL_VSS_H -row IO_NORTH -location 750.0 u_vss_11
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 720.0 u_co_2_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 685.0 u_co_1_i
place_pad -master PADCELL_SIG_V -row IO_NORTH -location 585.0 u_co_0_i
place_pad -master PADCELL_VSSIO_H -row IO_NORTH -location 555.0 u_vzz_25
place_pad -master PADCELL_VDDIO_H -row IO_NORTH -location 395.0 u_v18_25
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2795.0 u_co2_8_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2670.0 u_co2_7_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2635.0 u_co2_6_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2505.0 u_co2_5_o
place_pad -master PADCELL_VDD_H -row IO_WEST -location 2475.0 u_vdd_12
place_pad -master PADCELL_VSS_H -row IO_WEST -location 2445.0 u_vss_12
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 2385.0 u_vzz_26
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 2355.0 u_v18_26
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2325.0 u_co2_v_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2295.0 u_co2_tkn_i
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2215.0 u_co2_clk_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2185.0 u_co2_4_o
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 2155.0 u_vzz_27
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 2125.0 u_v18_27
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2095.0 u_co2_3_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2060.0 u_co2_2_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 2025.0 u_co2_1_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1995.0 u_co2_0_o
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 1965.0 u_vzz_28
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 1935.0 u_v18_28
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1900.0 u_bsg_tag_clk_o
place_pad -master PADCELL_VDD_H -row IO_WEST -location 1870.0 u_vdd_13
place_pad -master PADCELL_VSS_H -row IO_WEST -location 1840.0 u_vss_13
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1810.0 u_bsg_tag_data_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1775.0 u_ddr_dq_7_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1735.0 u_ddr_dq_6_io
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 1705.0 u_vzz_29
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 1675.0 u_v18_29
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1645.0 u_ddr_dq_5_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1615.0 u_ddr_dq_4_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1575.0 u_ddr_dq_3_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1545.0 u_ddr_dq_2_io
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 1515.0 u_vzz_30
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 1485.0 u_v18_30
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1455.0 u_ddr_dq_1_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1420.0 u_ddr_dq_0_io
place_pad -master PADCELL_VDD_H -row IO_WEST -location 1390.0 u_vdd_14
place_pad -master PADCELL_VSS_H -row IO_WEST -location 1360.0 u_vss_14
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1325.0 u_ddr_dm_0_o
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1295.0 u_ddr_dqs_n_0_io
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 1260.0 u_vzz_31
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 1230.0 u_v18_31
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1200.0 u_ddr_dqs_p_0_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1170.0 u_ddr_dq_15_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1135.0 u_ddr_dq_14_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1095.0 u_ddr_dq_13_io
place_pad -master PADCELL_VSSIO_H -row IO_WEST -location 1065.0 u_vzz_32
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 1035.0 u_v18_32
place_pad -master PADCELL_SIG_H -row IO_WEST -location 1005.0 u_ddr_dq_12_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 975.0 u_ddr_dq_11_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 935.0 u_ddr_dq_10_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 905.0 u_ddr_dq_9_io
place_pad -master PADCELL_SIG_H -row IO_WEST -location 875.0 u_ddr_dq_8_io
place_pad -master PADCELL_VDD_H -row IO_WEST -location 845.0 u_vdd_15
place_pad -master PADCELL_VSS_H -row IO_WEST -location 745.0 u_vss_15
place_pad -master PADCELL_VSSIO_V -row IO_WEST -location 715.0 u_vzz_33
place_pad -master PADCELL_VDDIO_H -row IO_WEST -location 685.0 u_v18_33

global_connect

# Place corners
place_corners PAD_CORNER

# Place Fill
place_io_fill -row IO_NORTH PAD_FILL5_H PAD_FILL1_H
place_io_fill -row IO_SOUTH PAD_FILL5_H PAD_FILL1_H
place_io_fill -row IO_EAST PAD_FILL5_V PAD_FILL1_V
place_io_fill -row IO_WEST PAD_FILL5_V PAD_FILL1_V

# Connect ring
connect_by_abutment

# Make bump array
make_io_bump_array -bump DUMMY_BUMP_TWO_PORTS -origin "210.0 215.0" -pitch "160 160" -rows 17 -columns 17
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
assign_io_bump -net DVSS BUMP_1_0
assign_io_bump -net DVDD BUMP_2_1
assign_io_bump -net p_ddr_dqs_p_1_io -terminal u_ddr_dqs_p_1_io/PAD BUMP_2_0
assign_io_bump -net VDD BUMP_2_2
assign_io_bump -net VSS BUMP_3_3
assign_io_bump -net p_ddr_ba_2_o -terminal u_ddr_ba_2_o/PAD BUMP_3_1
assign_io_bump -net p_ddr_ba_1_o -terminal u_ddr_ba_1_o/PAD BUMP_3_0
assign_io_bump -net p_ddr_ba_0_o -terminal u_ddr_ba_0_o/PAD BUMP_3_2
assign_io_bump -net DVSS BUMP_4_3
assign_io_bump -net DVDD BUMP_4_1
assign_io_bump -net p_ddr_addr_15_o -terminal u_ddr_addr_15_o/PAD BUMP_4_0
assign_io_bump -net p_ddr_addr_14_o -terminal u_ddr_addr_14_o/PAD BUMP_4_2
assign_io_bump -net p_ddr_addr_13_o -terminal u_ddr_addr_13_o/PAD BUMP_4_4
assign_io_bump -net p_ddr_addr_12_o -terminal u_ddr_addr_12_o/PAD BUMP_5_3
assign_io_bump -net DVSS BUMP_5_1
assign_io_bump -net DVDD BUMP_5_0
assign_io_bump -net p_ddr_addr_11_o -terminal u_ddr_addr_11_o/PAD BUMP_5_2
assign_io_bump -net p_ddr_addr_10_o -terminal u_ddr_addr_10_o/PAD BUMP_5_4
assign_io_bump -net VDD BUMP_6_3
assign_io_bump -net VSS BUMP_6_1
assign_io_bump -net p_ddr_addr_9_o -terminal u_ddr_addr_9_o/PAD BUMP_6_0
assign_io_bump -net p_ddr_addr_8_o -terminal u_ddr_addr_8_o/PAD BUMP_6_2
assign_io_bump -net DVSS BUMP_6_4
assign_io_bump -net DVDD BUMP_7_3
assign_io_bump -net p_ddr_addr_7_o -terminal u_ddr_addr_7_o/PAD BUMP_7_1
assign_io_bump -net p_ddr_addr_6_o -terminal u_ddr_addr_6_o/PAD BUMP_7_0
assign_io_bump -net p_ddr_addr_5_o -terminal u_ddr_addr_5_o/PAD BUMP_7_2
assign_io_bump -net p_ddr_addr_4_o -terminal u_ddr_addr_4_o/PAD BUMP_7_4
assign_io_bump -net DVSS BUMP_8_3
assign_io_bump -net DVDD BUMP_8_1
assign_io_bump -net p_ddr_addr_3_o -terminal u_ddr_addr_3_o/PAD BUMP_8_0
assign_io_bump -net p_ddr_addr_2_o -terminal u_ddr_addr_2_o/PAD BUMP_8_2
assign_io_bump -net p_ddr_addr_1_o -terminal u_ddr_addr_1_o/PAD BUMP_8_4
assign_io_bump -net VDD BUMP_9_3
assign_io_bump -net VSS BUMP_9_1
assign_io_bump -net p_ddr_addr_0_o -terminal u_ddr_addr_0_o/PAD BUMP_9_0
assign_io_bump -net DVSS BUMP_9_2
assign_io_bump -net DVDD BUMP_9_4
assign_io_bump -net p_ddr_odt_o -terminal u_ddr_odt_o/PAD BUMP_10_3
assign_io_bump -net p_ddr_reset_n_o -terminal u_ddr_reset_n_o/PAD BUMP_10_1
assign_io_bump -net p_ddr_we_n_o -terminal u_ddr_we_n_o/PAD BUMP_10_0
assign_io_bump -net p_ddr_cas_n_o -terminal u_ddr_cas_n_o/PAD BUMP_10_2
assign_io_bump -net DVSS BUMP_10_4
assign_io_bump -net DVDD BUMP_11_3
assign_io_bump -net p_ddr_ras_n_o -terminal u_ddr_ras_n_o/PAD BUMP_11_1
assign_io_bump -net p_ddr_cs_n_o -terminal u_ddr_cs_n_o/PAD BUMP_11_0
assign_io_bump -net p_ddr_cke_o -terminal u_ddr_cke_o/PAD BUMP_11_2
assign_io_bump -net p_ddr_ck_n_o -terminal u_ddr_ck_n_o/PAD BUMP_11_4
assign_io_bump -net DVSS BUMP_12_3
assign_io_bump -net DVDD BUMP_12_1
assign_io_bump -net VDD BUMP_12_0
assign_io_bump -net VSS BUMP_12_2
assign_io_bump -net p_ddr_ck_p_o -terminal u_ddr_ck_p_o/PAD BUMP_13_1
assign_io_bump -net p_ddr_dqs_n_2_io -terminal u_ddr_dqs_n_2_io/PAD BUMP_13_0
assign_io_bump -net p_ddr_dqs_p_2_io -terminal u_ddr_dqs_p_2_io/PAD BUMP_13_2
assign_io_bump -net p_ddr_dm_2_o -terminal u_ddr_dm_2_o/PAD BUMP_14_1
assign_io_bump -net DVSS BUMP_14_0
assign_io_bump -net DVDD BUMP_15_0
assign_io_bump -net p_ddr_dq_23_io -terminal u_ddr_dq_23_io/PAD BUMP_16_0
assign_io_bump -net p_ddr_dq_22_io -terminal u_ddr_dq_22_io/PAD BUMP_15_1
assign_io_bump -net p_ddr_dq_21_io -terminal u_ddr_dq_21_io/PAD BUMP_16_1
assign_io_bump -net VDD BUMP_15_2
assign_io_bump -net VSS BUMP_16_2
assign_io_bump -net p_ddr_dq_20_io -terminal u_ddr_dq_20_io/PAD BUMP_14_2
assign_io_bump -net DVSS BUMP_13_3
assign_io_bump -net DVDD BUMP_15_3
assign_io_bump -net p_ddr_dq_19_io -terminal u_ddr_dq_19_io/PAD BUMP_16_3
assign_io_bump -net p_ddr_dq_18_io -terminal u_ddr_dq_18_io/PAD BUMP_14_3
assign_io_bump -net p_ddr_dq_17_io -terminal u_ddr_dq_17_io/PAD BUMP_13_4
assign_io_bump -net p_ddr_dq_16_io -terminal u_ddr_dq_16_io/PAD BUMP_15_4
assign_io_bump -net DVSS BUMP_16_4
assign_io_bump -net DVDD BUMP_14_4
assign_io_bump -net p_ddr_dq_31_io -terminal u_ddr_dq_31_io/PAD BUMP_12_4
assign_io_bump -net p_ddr_dq_30_io -terminal u_ddr_dq_30_io/PAD BUMP_13_5
assign_io_bump -net p_ddr_dq_29_io -terminal u_ddr_dq_29_io/PAD BUMP_15_5
assign_io_bump -net p_ddr_dq_28_io -terminal u_ddr_dq_28_io/PAD BUMP_16_5
assign_io_bump -net DVSS BUMP_14_5
assign_io_bump -net DVDD BUMP_12_5
assign_io_bump -net p_ddr_dq_27_io -terminal u_ddr_dq_27_io/PAD BUMP_13_6
assign_io_bump -net VDD BUMP_15_6
assign_io_bump -net VSS BUMP_16_6
assign_io_bump -net p_ddr_dq_26_io -terminal u_ddr_dq_26_io/PAD BUMP_14_6
assign_io_bump -net p_ddr_dq_25_io -terminal u_ddr_dq_25_io/PAD BUMP_12_6
assign_io_bump -net p_ddr_dq_24_io -terminal u_ddr_dq_24_io/PAD BUMP_13_7
assign_io_bump -net DVSS BUMP_15_7
assign_io_bump -net DVDD BUMP_16_7
assign_io_bump -net p_ddr_dqs_n_3_io -terminal u_ddr_dqs_n_3_io/PAD BUMP_14_7
assign_io_bump -net p_ddr_dqs_p_3_io -terminal u_ddr_dqs_p_3_io/PAD BUMP_12_7
assign_io_bump -net p_ddr_dm_3_o -terminal u_ddr_dm_3_o/PAD BUMP_13_8
assign_io_bump -net p_bsg_tag_clk_i -terminal u_bsg_tag_clk_i/PAD BUMP_15_8
assign_io_bump -net DVSS BUMP_16_8
assign_io_bump -net DVDD BUMP_14_8
assign_io_bump -net p_bsg_tag_data_i -terminal u_bsg_tag_data_i/PAD BUMP_12_8
assign_io_bump -net p_bsg_tag_en_i -terminal u_bsg_tag_en_i/PAD BUMP_13_9
assign_io_bump -net p_ci_8_i -terminal u_ci_8_i/PAD BUMP_15_9
assign_io_bump -net VDD BUMP_16_9
assign_io_bump -net VSS BUMP_14_9
assign_io_bump -net p_ci_7_i -terminal u_ci_7_i/PAD BUMP_12_9
assign_io_bump -net DVSS BUMP_13_10
assign_io_bump -net DVDD BUMP_15_10
assign_io_bump -net p_ci_6_i -terminal u_ci_6_i/PAD BUMP_16_10
assign_io_bump -net p_ci_5_i -terminal u_ci_5_i/PAD BUMP_14_10
assign_io_bump -net p_ci_v_i -terminal u_ci_v_i/PAD BUMP_12_10
assign_io_bump -net p_ci_tkn_o -terminal u_ci_tkn_o/PAD BUMP_13_11
assign_io_bump -net DVSS BUMP_15_11
assign_io_bump -net DVDD BUMP_16_11
assign_io_bump -net p_ci_clk_i -terminal u_ci_clk_i/PAD BUMP_14_11
assign_io_bump -net p_ci_4_i -terminal u_ci_4_i/PAD BUMP_12_11
assign_io_bump -net p_ci_3_i -terminal u_ci_3_i/PAD BUMP_13_12
assign_io_bump -net p_ci_2_i -terminal u_ci_2_i/PAD BUMP_15_12
assign_io_bump -net DVSS BUMP_16_12
assign_io_bump -net DVDD BUMP_14_12
assign_io_bump -net p_ci_1_i -terminal u_ci_1_i/PAD BUMP_15_13
assign_io_bump -net VDD BUMP_16_13
assign_io_bump -net VSS BUMP_14_13
assign_io_bump -net p_ci_0_i -terminal u_ci_0_i/PAD BUMP_15_14
assign_io_bump -net p_ci2_8_o -terminal u_ci2_8_o/PAD BUMP_16_14
assign_io_bump -net p_ci2_7_o -terminal u_ci2_7_o/PAD BUMP_16_15
assign_io_bump -net DVSS BUMP_16_16
assign_io_bump -net DVDD BUMP_15_15
assign_io_bump -net p_ci2_6_o -terminal u_ci2_6_o/PAD BUMP_15_16
assign_io_bump -net p_ci2_5_o -terminal u_ci2_5_o/PAD BUMP_14_15
assign_io_bump -net VDD BUMP_14_16
assign_io_bump -net VSS BUMP_14_14
assign_io_bump -net p_ci2_v_o -terminal u_ci2_v_o/PAD BUMP_13_13
assign_io_bump -net p_ci2_tkn_i -terminal u_ci2_tkn_i/PAD BUMP_13_15
assign_io_bump -net DVSS BUMP_13_16
assign_io_bump -net DVDD BUMP_13_14
assign_io_bump -net p_ci2_clk_o -terminal u_ci2_clk_o/PAD BUMP_12_13
assign_io_bump -net p_ci2_4_o -terminal u_ci2_4_o/PAD BUMP_12_15
assign_io_bump -net p_ci2_3_o -terminal u_ci2_3_o/PAD BUMP_12_16
assign_io_bump -net p_ci2_2_o -terminal u_ci2_2_o/PAD BUMP_12_14
assign_io_bump -net DVSS BUMP_12_12
assign_io_bump -net DVDD BUMP_11_13
assign_io_bump -net p_ci2_1_o -terminal u_ci2_1_o/PAD BUMP_11_15
assign_io_bump -net p_ci2_0_o -terminal u_ci2_0_o/PAD BUMP_11_16
assign_io_bump -net p_core_async_reset_i -terminal u_core_async_reset_i/PAD BUMP_11_14
assign_io_bump -net VDD BUMP_11_12
assign_io_bump -net VSS BUMP_10_13
assign_io_bump -net p_sel_2_i -terminal u_sel_2_i/PAD BUMP_10_15
assign_io_bump -net DVDD BUMP_10_16
assign_io_bump -net DVSS BUMP_10_14
assign_io_bump -net p_sel_1_i -terminal u_sel_1_i/PAD BUMP_10_12
assign_io_bump -net p_sel_0_i -terminal u_sel_0_i/PAD BUMP_9_13
assign_io_bump -net p_misc_o -terminal u_misc_o/PAD BUMP_9_15
assign_io_bump -net VSS BUMP_9_16
assign_io_bump -net p_clk_async_reset_i -terminal u_clk_async_reset_i/PAD BUMP_9_14
assign_io_bump -net VDD BUMP_9_12
assign_io_bump -net p_clk_o -terminal u_clk_o/PAD BUMP_8_13
assign_io_bump -net DVSS BUMP_8_15
assign_io_bump -net p_clk_C_i -terminal u_clk_C_i/PAD BUMP_8_16
assign_io_bump -net DVDD BUMP_8_14
assign_io_bump -net p_clk_B_i -terminal u_clk_B_i/PAD BUMP_8_12
assign_io_bump -net p_clk_A_i -terminal u_clk_A_i/PAD BUMP_7_13
assign_io_bump -net VDD BUMP_7_15
assign_io_bump -net VSS BUMP_7_16
assign_io_bump -net DVSS BUMP_7_14
assign_io_bump -net DVDD BUMP_7_12
assign_io_bump -net p_co_8_i -terminal u_co_8_i/PAD BUMP_6_13
assign_io_bump -net p_co_7_i -terminal u_co_7_i/PAD BUMP_6_15
assign_io_bump -net p_co_6_i -terminal u_co_6_i/PAD BUMP_6_16
assign_io_bump -net p_co_5_i -terminal u_co_5_i/PAD BUMP_6_14
assign_io_bump -net DVSS BUMP_6_12
assign_io_bump -net DVDD BUMP_5_13
assign_io_bump -net p_co_v_i -terminal u_co_v_i/PAD BUMP_5_15
assign_io_bump -net p_co_tkn_o -terminal u_co_tkn_o/PAD BUMP_5_16
assign_io_bump -net p_co_clk_i -terminal u_co_clk_i/PAD BUMP_5_14
assign_io_bump -net p_co_4_i -terminal u_co_4_i/PAD BUMP_5_12
assign_io_bump -net DVSS BUMP_4_13
assign_io_bump -net DVDD BUMP_4_15
assign_io_bump -net p_co_3_i -terminal u_co_3_i/PAD BUMP_4_16
assign_io_bump -net VDD BUMP_4_14
assign_io_bump -net VSS BUMP_3_15
assign_io_bump -net p_co_2_i -terminal u_co_2_i/PAD BUMP_3_16
assign_io_bump -net p_co_1_i -terminal u_co_1_i/PAD BUMP_3_14
assign_io_bump -net p_co_0_i -terminal u_co_0_i/PAD BUMP_2_15
assign_io_bump -net DVSS BUMP_2_16
assign_io_bump -net DVDD BUMP_1_16
assign_io_bump -net p_co2_8_o -terminal u_co2_8_o/PAD BUMP_0_16
assign_io_bump -net p_co2_7_o -terminal u_co2_7_o/PAD BUMP_1_15
assign_io_bump -net p_co2_6_o -terminal u_co2_6_o/PAD BUMP_0_15
assign_io_bump -net p_co2_5_o -terminal u_co2_5_o/PAD BUMP_1_14
assign_io_bump -net VDD BUMP_0_14
assign_io_bump -net VSS BUMP_2_14
assign_io_bump -net DVSS BUMP_3_13
assign_io_bump -net DVDD BUMP_1_13
assign_io_bump -net p_co2_v_o -terminal u_co2_v_o/PAD BUMP_0_13
assign_io_bump -net p_co2_tkn_i -terminal u_co2_tkn_i/PAD BUMP_2_13
assign_io_bump -net p_co2_clk_o -terminal u_co2_clk_o/PAD BUMP_3_12
assign_io_bump -net p_co2_4_o -terminal u_co2_4_o/PAD BUMP_1_12
assign_io_bump -net DVSS BUMP_0_12
assign_io_bump -net DVDD BUMP_2_12
assign_io_bump -net p_co2_3_o -terminal u_co2_3_o/PAD BUMP_4_12
assign_io_bump -net p_co2_2_o -terminal u_co2_2_o/PAD BUMP_3_11
assign_io_bump -net p_co2_1_o -terminal u_co2_1_o/PAD BUMP_1_11
assign_io_bump -net p_co2_0_o -terminal u_co2_0_o/PAD BUMP_0_11
assign_io_bump -net DVSS BUMP_2_11
assign_io_bump -net DVDD BUMP_4_11
assign_io_bump -net p_bsg_tag_clk_o -terminal u_bsg_tag_clk_o/PAD BUMP_3_10
assign_io_bump -net VDD BUMP_1_10
assign_io_bump -net VSS BUMP_0_10
assign_io_bump -net p_bsg_tag_data_o -terminal u_bsg_tag_data_o/PAD BUMP_2_10
assign_io_bump -net p_ddr_dq_7_io -terminal u_ddr_dq_7_io/PAD BUMP_4_10
assign_io_bump -net p_ddr_dq_6_io -terminal u_ddr_dq_6_io/PAD BUMP_3_9
assign_io_bump -net DVSS BUMP_1_9
assign_io_bump -net DVDD BUMP_0_9
assign_io_bump -net p_ddr_dq_5_io -terminal u_ddr_dq_5_io/PAD BUMP_2_9
assign_io_bump -net p_ddr_dq_4_io -terminal u_ddr_dq_4_io/PAD BUMP_4_9
assign_io_bump -net p_ddr_dq_3_io -terminal u_ddr_dq_3_io/PAD BUMP_3_8
assign_io_bump -net p_ddr_dq_2_io -terminal u_ddr_dq_2_io/PAD BUMP_1_8
assign_io_bump -net DVSS BUMP_0_8
assign_io_bump -net DVDD BUMP_2_8
assign_io_bump -net p_ddr_dq_1_io -terminal u_ddr_dq_1_io/PAD BUMP_4_8
assign_io_bump -net p_ddr_dq_0_io -terminal u_ddr_dq_0_io/PAD BUMP_3_7
assign_io_bump -net VDD BUMP_1_7
assign_io_bump -net VSS BUMP_0_7
assign_io_bump -net p_ddr_dm_0_o -terminal u_ddr_dm_0_o/PAD BUMP_2_7
assign_io_bump -net p_ddr_dqs_n_0_io -terminal u_ddr_dqs_n_0_io/PAD BUMP_4_7
assign_io_bump -net DVSS BUMP_3_6
assign_io_bump -net DVDD BUMP_1_6
assign_io_bump -net p_ddr_dqs_p_0_io -terminal u_ddr_dqs_p_0_io/PAD BUMP_0_6
assign_io_bump -net p_ddr_dq_15_io -terminal u_ddr_dq_15_io/PAD BUMP_2_6
assign_io_bump -net p_ddr_dq_14_io -terminal u_ddr_dq_14_io/PAD BUMP_4_6
assign_io_bump -net p_ddr_dq_13_io -terminal u_ddr_dq_13_io/PAD BUMP_3_5
assign_io_bump -net DVSS BUMP_1_5
assign_io_bump -net DVDD BUMP_0_5
assign_io_bump -net p_ddr_dq_12_io -terminal u_ddr_dq_12_io/PAD BUMP_2_5
assign_io_bump -net p_ddr_dq_11_io -terminal u_ddr_dq_11_io/PAD BUMP_4_5
assign_io_bump -net p_ddr_dq_10_io -terminal u_ddr_dq_10_io/PAD BUMP_3_4
assign_io_bump -net p_ddr_dq_9_io -terminal u_ddr_dq_9_io/PAD BUMP_1_4
assign_io_bump -net p_ddr_dq_8_io -terminal u_ddr_dq_8_io/PAD BUMP_0_4
assign_io_bump -net VDD BUMP_2_4
assign_io_bump -net VSS BUMP_1_3
assign_io_bump -net DVSS BUMP_0_3
assign_io_bump -net DVDD BUMP_2_3
assign_io_bump -net VDD BUMP_2_2
assign_io_bump -net VDD BUMP_6_3
assign_io_bump -net VDD BUMP_9_3
assign_io_bump -net VDD BUMP_12_0
assign_io_bump -net VDD BUMP_12_0
assign_io_bump -net DVSS BUMP_16_16

assign_io_bump -net DVSS BUMP_5_5 -dont_route
assign_io_bump -net DVSS BUMP_5_6 -dont_route
assign_io_bump -net DVSS BUMP_5_7 -dont_route
assign_io_bump -net DVSS BUMP_6_5 -dont_route
assign_io_bump -net DVSS BUMP_6_6 -dont_route
assign_io_bump -net DVSS BUMP_6_7 -dont_route
assign_io_bump -net DVSS BUMP_7_5 -dont_route
assign_io_bump -net DVSS BUMP_7_6 -dont_route
assign_io_bump -net DVSS BUMP_7_7 -dont_route

assign_io_bump -net DVDD BUMP_5_9 -dont_route
assign_io_bump -net DVDD BUMP_5_10 -dont_route
assign_io_bump -net DVDD BUMP_5_11 -dont_route
assign_io_bump -net DVDD BUMP_6_9 -dont_route
assign_io_bump -net DVDD BUMP_6_10 -dont_route
assign_io_bump -net DVDD BUMP_6_11 -dont_route
assign_io_bump -net DVDD BUMP_7_9 -dont_route
assign_io_bump -net DVDD BUMP_7_10 -dont_route
assign_io_bump -net DVDD BUMP_7_11 -dont_route

assign_io_bump -net VSS BUMP_9_5 -dont_route
assign_io_bump -net VSS BUMP_9_6 -dont_route
assign_io_bump -net VSS BUMP_9_7 -dont_route
assign_io_bump -net VSS BUMP_10_5 -dont_route
assign_io_bump -net VSS BUMP_10_6 -dont_route
assign_io_bump -net VSS BUMP_10_7 -dont_route
assign_io_bump -net VSS BUMP_11_5 -dont_route
assign_io_bump -net VSS BUMP_11_6 -dont_route
assign_io_bump -net VSS BUMP_11_7 -dont_route

assign_io_bump -net VDD BUMP_9_9 -dont_route
assign_io_bump -net VDD BUMP_9_10 -dont_route
assign_io_bump -net VDD BUMP_9_11 -dont_route
assign_io_bump -net VDD BUMP_10_9 -dont_route
assign_io_bump -net VDD BUMP_10_10 -dont_route
assign_io_bump -net VDD BUMP_10_11 -dont_route
assign_io_bump -net VDD BUMP_11_9 -dont_route
assign_io_bump -net VDD BUMP_11_10 -dont_route
assign_io_bump -net VDD BUMP_11_11 -dont_route

set def_file [make_result_file "assign_bumps_two_pins.def"]
write_def $def_file
diff_files $def_file "assign_bumps_two_pins.defok"
