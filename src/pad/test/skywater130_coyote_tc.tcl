source "helpers.tcl"

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef

read_lef skywater130_io_fd/lef/sky130_fd_io__corner_bus_overlay.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_gpiov2.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vccd_hvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vccd_lvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vdda_hvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vdda_lvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vddio_hvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vddio_lvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vssa_hvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vssa_lvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vssd_hvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vssd_lvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vssio_hvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__overlay_vssio_lvc.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_gpio_ovtv2.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_gpiov2.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_ground_hvc_wpad.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_ground_lvc_wpad.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_power_hvc_wpad.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_power_lvc_wpad.lef
read_lef skywater130_io_fd/lef/sky130_fd_io__top_xres4v2.lef
read_lef skywater130_io_fd/lef/sky130io_fill.lef

read_liberty sky130hs/sky130hs_tt.lib
read_liberty skywater130_io_fd/lib/sky130_dummy_io.lib

read_verilog skywater130_coyote_tc/coyote_tc.v

link_design coyote_tc

initialize_floorplan \
  -die_area  "0.0 0.0 5400.0 4616.330" \
  -core_area "200.16 203.13 5199.84 4415.58" \
  -site      unit

make_tracks

make_fake_io_site \
  -name IO_SITE \
  -width 1 \
  -height 200
make_fake_io_site \
  -name IO_CSITE \
  -width 200 \
  -height 203.665
make_io_sites \
  -horizontal_site IO_SITE \
  -vertical_site IO_SITE \
  -corner_site IO_CSITE \
  \
  -offset 0 \
  -rotation_horizontal R180 \
  -rotation_vertical R180 \
  -rotation_corner R180

######## Place Pads ########
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 280.0 {u_clk.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 360.0 {u_reset.u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 439.5 {u_vzz_0}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 517.5 {u_v18_0}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 680.0 {u_rocc_cmd_v.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 760.0 {u_rocc_cmd_data_o_0_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 840.0 {u_rocc_cmd_data_o_1_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 920.0 {u_rocc_cmd_data_o_2_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1000.0 {u_rocc_cmd_data_o_3_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1080.0 {u_rocc_cmd_data_o_4_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1160.0 {u_rocc_cmd_data_o_5_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 1239.5 {u_vzz_1}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 1317.5 {u_v18_1}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1400.0 {u_rocc_cmd_data_o_6_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1480.0 {u_rocc_cmd_data_o_7_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1560.0 {u_rocc_cmd_data_o_8_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 1639.5 {u_vss_0}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 1717.5 {u_vdd_0}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1800.0 {u_rocc_cmd_data_o_9_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1880.0 {u_rocc_cmd_data_o_10_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 1960.0 {u_rocc_cmd_data_o_11_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2040.0 {u_rocc_cmd_data_o_12_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2120.0 {u_rocc_cmd_data_o_13_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 2199.5 {u_vzz_2}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 2277.5 {u_v18_2}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2360.0 {u_rocc_cmd_data_o_14_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2440.0 {u_rocc_cmd_data_o_15_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2520.0 {u_rocc_cmd_ready.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2600.0 {u_rocc_resp_v.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2680.0 {u_rocc_resp_data_i.u_io\[0\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2760.0 {u_rocc_resp_data_i.u_io\[1\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2840.0 {u_rocc_resp_data_i.u_io\[2\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 2920.0 {u_rocc_resp_data_i.u_io\[3\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 2999.5 {u_vzz_3}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 3077.5 {u_v18_3}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3160.0 {u_rocc_resp_data_i.u_io\[4\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3240.0 {u_rocc_resp_data_i.u_io\[5\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3320.0 {u_rocc_resp_data_i.u_io\[6\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3400.0 {u_rocc_resp_data_i.u_io\[7\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3480.0 {u_rocc_resp_ready.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 3559.5 {u_vss_1}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 3637.5 {u_vdd_1}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3720.0 {u_rocc_mem_req_v.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3800.0 {u_rocc_mem_req_data_i.u_io\[0\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 3880.0 {u_rocc_mem_req_data_i.u_io\[1\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 3959.5 {u_vzz_4}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 4037.5 {u_v18_4}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4120.0 {u_rocc_mem_req_data_i.u_io\[2\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4200.0 {u_rocc_mem_req_data_i.u_io\[3\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4280.0 {u_rocc_mem_req_data_i.u_io\[4\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4360.0 {u_rocc_mem_req_data_i.u_io\[5\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4440.0 {u_rocc_mem_req_data_i.u_io\[6\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4520.0 {u_rocc_mem_req_data_i.u_io\[7\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4600.0 {u_rocc_mem_req_data_i.u_io\[8\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4680.0 {u_rocc_mem_req_data_i.u_io\[9\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_SOUTH \
  -location 4759.5 {u_vzz_5}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_SOUTH \
  -location 4837.5 {u_v18_5}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 4920.0 {u_rocc_mem_req_data_i.u_io\[10\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_SOUTH \
  -location 5000.0 {u_rocc_mem_req_data_i.u_io\[11\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 240.0 {u_rocc_mem_req_data_i.u_io\[12\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 320.0 {u_rocc_mem_req_data_i.u_io\[13\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 400.0 {u_rocc_mem_req_data_i.u_io\[14\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 480.0 {u_rocc_mem_req_data_i.u_io\[15\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 559.5 {u_vzz_6}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 637.5 {u_v18_6}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 717.5 {u_vss_2}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 797.5 {u_vdd_2}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 880.0 {u_rocc_mem_req_data_i.u_io\[16\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 960.0 {u_rocc_mem_req_data_i.u_io\[17\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1040.0 {u_rocc_mem_req_data_i.u_io\[18\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1120.0 {u_rocc_mem_req_data_i.u_io\[19\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1200.0 {u_rocc_mem_req_data_i.u_io\[20\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1280.0 {u_rocc_mem_req_data_i.u_io\[21\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1360.0 {u_rocc_mem_req_data_i.u_io\[22\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1440.0 {u_rocc_mem_req_data_i.u_io\[23\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 1519.5 {u_vzz_7}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 1597.5 {u_v18_7}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1680.0 {u_rocc_mem_req_data_i.u_io\[24\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1760.0 {u_rocc_mem_req_data_i.u_io\[25\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1840.0 {u_rocc_mem_req_data_i.u_io\[26\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 1920.0 {u_rocc_mem_req_data_i.u_io\[27\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2000.0 {u_rocc_mem_req_data_i.u_io\[28\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2080.0 {u_rocc_mem_req_data_i.u_io\[29\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2160.0 {u_rocc_mem_req_data_i.u_io\[30\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2240.0 {u_rocc_mem_req_data_i.u_io\[31\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 2319.5 {u_vzz_8}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 2397.5 {u_v18_8}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2480.0 {u_rocc_mem_req_ready.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2560.0 {u_rocc_mem_resp_v.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2640.0 {u_rocc_mem_resp_data_o_0_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 2719.5 {u_vss_3}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 2797.5 {u_vdd_3}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2880.0 {u_rocc_mem_resp_data_o_1_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 2960.0 {u_rocc_mem_resp_data_o_2_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3040.0 {u_rocc_mem_resp_data_o_3_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3120.0 {u_rocc_mem_resp_data_o_4_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3200.0 {u_rocc_mem_resp_data_o_5_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 3279.5 {u_vzz_9}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 3357.5 {u_v18_9}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3440.0 {u_rocc_mem_resp_data_o_6_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3520.0 {u_rocc_mem_resp_data_o_7_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3600.0 {u_rocc_mem_resp_data_o_8_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3680.0 {u_rocc_mem_resp_data_o_9_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3760.0 {u_rocc_mem_resp_data_o_10_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3840.0 {u_rocc_mem_resp_data_o_11_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 3920.0 {u_rocc_mem_resp_data_o_12_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 4000.0 {u_rocc_mem_resp_data_o_13_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_EAST \
  -location 4079.5 {u_vzz_10}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_EAST \
  -location 4157.5 {u_v18_10}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 4240.0 {u_rocc_mem_resp_data_o_14_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_EAST \
  -location 4320.0 {u_rocc_mem_resp_data_o_15_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4920.0 {u_rocc_mem_resp_data_o_16_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4840.0 {u_rocc_mem_resp_data_o_17_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4760.0 {u_rocc_mem_resp_data_o_18_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4680.0 {u_rocc_mem_resp_data_o_19_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 4602.5 {u_vss_4}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 4522.5 {u_vdd_4}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4440.0 {u_rocc_mem_resp_data_o_20_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4360.0 {u_rocc_mem_resp_data_o_21_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 4282.5 {u_vzz_11}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 4202.5 {u_v18_11}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4120.0 {u_rocc_mem_resp_data_o_22_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 4040.0 {u_rocc_mem_resp_data_o_23_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3960.0 {u_rocc_mem_resp_data_o_24_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3880.0 {u_rocc_mem_resp_data_o_25_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3800.0 {u_rocc_mem_resp_data_o_26_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3720.0 {u_rocc_mem_resp_data_o_27_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3640.0 {u_rocc_mem_resp_data_o_28_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3560.0 {u_rocc_mem_resp_data_o_29_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 3482.5 {u_vzz_12}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 3402.5 {u_v18_12}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3320.0 {u_rocc_mem_resp_data_o_30_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3240.0 {u_rocc_mem_resp_data_o_31_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3160.0 {u_rocc_mem_resp_data_o_32_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3080.0 {u_rocc_mem_resp_data_o_33_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 3000.0 {u_rocc_mem_resp_data_o_34_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2920.0 {u_rocc_mem_resp_data_o_35_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2840.0 {u_rocc_mem_resp_data_o_36_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2760.0 {u_rocc_mem_resp_data_o_37_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 2682.5 {u_vzz_13}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 2602.5 {u_v18_13}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 2522.5 {u_vss_5}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 2442.5 {u_vdd_5}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2360.0 {u_rocc_mem_resp_data_o_38_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2280.0 {u_rocc_mem_resp_data_o_39_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2200.0 {u_rocc_mem_resp_data_o_40_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2120.0 {u_rocc_mem_resp_data_o_41_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 2040.0 {u_rocc_mem_resp_data_o_42_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1960.0 {u_rocc_mem_resp_data_o_43_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1880.0 {u_rocc_mem_resp_data_o_44_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1800.0 {u_rocc_mem_resp_data_o_45_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 1722.5 {u_vzz_14}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 1642.5 {u_v18_14}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1560.0 {u_rocc_mem_resp_data_o_46_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1480.0 {u_rocc_mem_resp_data_o_47_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1400.0 {u_rocc_mem_resp_data_o_48_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1320.0 {u_rocc_mem_resp_data_o_49_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1240.0 {u_rocc_mem_resp_data_o_50_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1160.0 {u_rocc_mem_resp_data_o_51_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1080.0 {u_rocc_mem_resp_data_o_52_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 1000.0 {u_rocc_mem_resp_data_o_53_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 922.5 {u_vzz_15}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 842.5 {u_v18_15}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 760.0 {u_rocc_mem_resp_data_o_54_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 680.0 {u_rocc_mem_resp_data_o_55_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_NORTH \
  -location 602.5 {u_vdd_6}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_NORTH \
  -location 522.5 {u_vss_6}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 440.0 {u_rocc_mem_resp_data_o_56_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 360.0 {u_rocc_mem_resp_data_o_57_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 280.0 {u_rocc_mem_resp_data_o_58_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_NORTH \
  -location 200.0 {u_rocc_mem_resp_data_o_59_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 4280.0 {u_rocc_mem_resp_data_o_60_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 4200.0 {u_rocc_mem_resp_data_o_61_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 4122.5 {u_vzz_16}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 4042.5 {u_v18_16}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3960.0 {u_rocc_mem_resp_data_o_62_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3880.0 {u_rocc_mem_resp_data_o_63_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3800.0 {u_fsb_node_v_i.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3720.0 {u_fsb_node_data_i.u_io\[0\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3640.0 {u_fsb_node_data_i.u_io\[1\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3560.0 {u_fsb_node_data_i.u_io\[2\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3480.0 {u_fsb_node_data_i.u_io\[3\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3400.0 {u_fsb_node_data_i.u_io\[4\].u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 3322.5 {u_vzz_17}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 3242.5 {u_v18_17}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3160.0 {u_fsb_node_data_i.u_io\[5\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3080.0 {u_fsb_node_data_i.u_io\[6\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 3000.0 {u_fsb_node_data_i.u_io\[7\].u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2920.0 {u_fsb_node_ready.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2840.0 {u_fsb_node_v_o.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2760.0 {u_fsb_node_data_o_0_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 2682.5 {u_vss_7}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 2602.5 {u_vdd_7}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2520.0 {u_fsb_node_data_o_1_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2440.0 {u_fsb_node_data_o_2_.u_io}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 2362.5 {u_vzz_18}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 2282.5 {u_v18_18}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2200.0 {u_fsb_node_data_o_3_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2120.0 {u_fsb_node_data_o_4_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 2040.0 {u_fsb_node_data_o_5_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1960.0 {u_fsb_node_data_o_6_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1880.0 {u_fsb_node_data_o_7_.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1800.0 {u_fsb_node_yumi.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1720.0 {u_rocc_ctrl_i_busy.u_in}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1640.0 {u_rocc_ctrl_i_interrupt.u_in}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 1562.5 {u_vzz_19}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 1482.5 {u_v18_19}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1400.0 {u_rocc_ctrl_o_s.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1320.0 {u_rocc_ctrl_o_exception.u_io}
place_pad \
  -master sky130_fd_io__top_gpiov2 \
  -row IO_WEST \
  -location 1240.0 {u_rocc_ctrl_o_host_id.u_io}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 762.5 {u_vdd_8}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 682.5 {u_vss_8}
place_pad \
  -master sky130_fd_io__top_ground_hvc_wpad \
  -row IO_WEST \
  -location 602.5 {u_vzz_20}
place_pad \
  -master sky130_fd_io__top_power_hvc_wpad \
  -row IO_WEST \
  -location 522.5 {u_v18_20}

place_corners sky130_fd_io__corner_bus_overlay

place_io_fill \
  -row IO_NORTH s8iom0s8_com_bus_slice_1um
place_io_fill \
  -row IO_SOUTH s8iom0s8_com_bus_slice_1um
place_io_fill \
  -row IO_WEST s8iom0s8_com_bus_slice_1um
place_io_fill \
  -row IO_EAST s8iom0s8_com_bus_slice_1um

connect_by_abutment

place_io_terminals u_*/PAD

set def_file [make_result_file "skywater130_coyote_tc.def"]
write_def $def_file
diff_files $def_file "skywater130_coyote_tc.defok"
