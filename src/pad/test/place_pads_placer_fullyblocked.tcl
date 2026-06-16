# Test for placing pads using place_pads placer mode
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

# Create blockage that blocks all of the row
create_obstruction -region {0 0 100 3000} -layer metal10

catch {
  place_pads -mode placer -row IO_WEST u_vss_15 u_vdd_15 u_ddr_dq_8_io u_ddr_dq_9_io \
    u_ddr_dq_10_io u_ddr_dq_11_io u_ddr_dq_12_io u_v18_32 u_vzz_32 \
    u_ddr_dq_13_io u_ddr_dq_14_io u_ddr_dq_15_io u_ddr_dqs_p_0_io \
    u_v18_31 u_vzz_31 u_ddr_dqs_n_0_io u_ddr_dm_0_o u_vss_14 u_vdd_14 \
    u_ddr_dq_0_io u_ddr_dq_1_io u_v18_30 u_vzz_30 u_ddr_dq_2_io \
    u_ddr_dq_3_io u_ddr_dq_4_io u_ddr_dq_5_io u_v18_29 u_vzz_29 \
    u_ddr_dq_6_io u_ddr_dq_7_io u_bsg_tag_data_o u_vss_13 u_vdd_13 \
    u_bsg_tag_clk_o u_v18_28 u_vzz_28 u_co2_0_o u_co2_1_o u_co2_2_o \
    u_co2_3_o u_v18_27 u_vzz_27 u_co2_4_o u_co2_clk_o u_co2_tkn_i \
    u_co2_v_o u_v18_26 u_vzz_26 u_vss_12 u_vdd_12 u_co2_5_o u_co2_6_o \
    u_co2_7_o u_co2_8_o
} err
puts $err

set def_file [make_result_file "place_pads_placer_fullyblocked.def"]
write_def $def_file
diff_files $def_file "place_pads_placer_fullyblocked.defok"
