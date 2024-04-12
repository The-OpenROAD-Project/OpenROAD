source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__com_bus_slice_10um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__com_bus_slice_1um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__com_bus_slice_20um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__com_bus_slice_5um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__corner_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__disconnect_vccd_slice_5um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__disconnect_vdda_slice_5um.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__gpiov2_pad_wrapped.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vccd_hvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vccd_lvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vdda_hvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vdda_lvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vddio_hvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vddio_lvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vssa_hvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vssa_lvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vssd_hvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vssd_lvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vssio_hvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_ef_io__vssio_lvc_pad.lef
read_lef skywater130_io_ef/lef/sky130_fd_io__top_xres4v2.lef

read_liberty skywater130_io_ef/lib/sky130_dummy_io.lib
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog skywater130_caravel/chip.v
link_design chip_io

initialize_floorplan \
  -die_area  "0 0 3588 5188" \
  -core_area "250 250 3338 4938" \
  -site    unithd

make_tracks

make_fake_io_site \
  -name IO_SITE \
  -width 1 \
  -height 200
make_fake_io_site \
  -name IO_CSITE \
  -width 200 \
  -height 204
make_io_sites -horizontal_site IO_SITE -vertical_site IO_SITE -corner_site IO_CSITE \
  -offset 0 -rotation_horizontal R180 -rotation_vertical R180 -rotation_corner R180

######## Place Pads ########
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 257.0 \
  {connect_0}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 277.0 \
  {connect_1}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 297.0 \
  {connect_2}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 317.0 \
  {connect_3}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 337.0 \
  {connect_4}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 357.0 \
  {connect_5}
place_pad \
  -master sky130_ef_io__vssa_hvc_pad \
  -row IO_SOUTH \
  -location 439.0 \
  {mgmt_vssa_hvclamp_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 526.0 \
  {connect_6}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 546.0 \
  {connect_7}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 566.0 \
  {connect_8}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 586.0 \
  {connect_9}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 606.0 \
  {connect_10}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 626.0 \
  {connect_11}
place_pad \
  -master sky130_fd_io__top_xres4v2 \
  -row IO_SOUTH \
  -location 708.0 \
  {resetb_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 795.0 \
  {connect_12}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 815.0 \
  {connect_13}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 835.0 \
  {connect_14}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 855.0 \
  {connect_15}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 875.0 \
  {connect_16}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 895.0 \
  {connect_17}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_SOUTH \
  -location 982.0 \
  {clock_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1069.0 \
  {connect_18}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1089.0 \
  {connect_19}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1109.0 \
  {connect_20}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1129.0 \
  {connect_21}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1149.0 \
  {connect_22}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1169.0 \
  {connect_23}
place_pad \
  -master sky130_ef_io__vssd_lvc_pad \
  -row IO_SOUTH \
  -location 1251.0 \
  {mgmt_vssd_lvclmap_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1338.0 \
  {connect_24}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1358.0 \
  {connect_25}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1378.0 \
  {connect_26}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1398.0 \
  {connect_27}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1418.0 \
  {connect_28}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1438.0 \
  {connect_29}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_SOUTH \
  -location 1525.0 \
  {flash_csb_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1612.0 \
  {connect_30}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1632.0 \
  {connect_31}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1652.0 \
  {connect_32}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1672.0 \
  {connect_33}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1692.0 \
  {connect_34}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1712.0 \
  {connect_35}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_SOUTH \
  -location 1799.0 \
  {flash_clk_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1886.0 \
  {connect_36}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1906.0 \
  {connect_37}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1926.0 \
  {connect_38}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1946.0 \
  {connect_39}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1966.0 \
  {connect_40}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 1986.0 \
  {connect_41}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_SOUTH \
  -location 2073.0 \
  {flash_io0_pad}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2160.0 \
  {connect_42}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2180.0 \
  {connect_43}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2200.0 \
  {connect_44}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2220.0 \
  {connect_45}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2240.0 \
  {connect_46}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2260.0 \
  {connect_47}
place_pad \
  -master sky130_ef_io__vdda_hvc_pad \
  -row IO_SOUTH \
  -location 3159.0 \
  {mgmt_vdda_hvclamp_pad}
place_pad \
  -master sky130_ef_io__vssio_hvc_pad \
  -row IO_SOUTH \
  -location 2890.0 \
  {mgmt_vssio_hvclamp_pad\[0\]}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2434.0 \
  {connect_48}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2454.0 \
  {connect_49}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2474.0 \
  {connect_50}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2494.0 \
  {connect_51}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2514.0 \
  {connect_52}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2534.0 \
  {connect_53}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2708.0 \
  {connect_54}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2728.0 \
  {connect_55}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2748.0 \
  {connect_56}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2768.0 \
  {connect_57}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2788.0 \
  {connect_58}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2808.0 \
  {connect_59}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2977.0 \
  {connect_60}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 2997.0 \
  {connect_61}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3017.0 \
  {connect_62}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3037.0 \
  {connect_63}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3057.0 \
  {connect_64}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3077.0 \
  {connect_65}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3246.0 \
  {connect_66}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3266.0 \
  {connect_67}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3286.0 \
  {connect_68}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3306.0 \
  {connect_69}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3326.0 \
  {connect_70}
place_pad \
  -master sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um \
  -row IO_SOUTH \
  -location 3346.0 \
  {connect_71}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_SOUTH \
  -location 2347.0 \
  {flash_io1_pad}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_SOUTH \
  -location 2621.0 \
  {gpio_pad}
place_pad \
  -master sky130_ef_io__disconnect_vdda_slice_5um \
  -row IO_EAST \
  -location 350.0 \
  {brk_vdda_0}
place_pad \
  -master sky130_ef_io__disconnect_vccd_slice_5um \
  -row IO_EAST \
  -location 355.0 \
  {brk_vccd_0}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 580.0 \
  {mprj_pads.area1_io_pad\[0\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 806.0 \
  {mprj_pads.area1_io_pad\[1\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 1031.0 \
  {mprj_pads.area1_io_pad\[2\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 1257.0 \
  {mprj_pads.area1_io_pad\[3\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 1482.0 \
  {mprj_pads.area1_io_pad\[4\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 1707.0 \
  {mprj_pads.area1_io_pad\[5\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 1933.0 \
  {mprj_pads.area1_io_pad\[6\]}
place_pad \
  -master sky130_ef_io__vssa_hvc_pad \
  -row IO_EAST \
  -location 2153.0 \
  {user1_vssa_hvclamp_pad\[1\]}
place_pad \
  -master sky130_ef_io__vssd_lvc_pad \
  -row IO_EAST \
  -location 2374.0 \
  {user1_vssd_lvclmap_pad}
place_pad \
  -master sky130_ef_io__vdda_hvc_pad \
  -row IO_EAST \
  -location 2594.0 \
  {user1_vdda_hvclamp_pad\[1\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 2819.0 \
  {mprj_pads.area1_io_pad\[7\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 3045.0 \
  {mprj_pads.area1_io_pad\[8\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 3270.0 \
  {mprj_pads.area1_io_pad\[9\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 3496.0 \
  {mprj_pads.area1_io_pad\[10\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 3721.0 \
  {mprj_pads.area1_io_pad\[11\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 3946.0 \
  {mprj_pads.area1_io_pad\[12\]}
place_pad \
  -master sky130_ef_io__vdda_hvc_pad \
  -row IO_EAST \
  -location 4167.0 \
  {user1_vdda_hvclamp_pad\[0\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 4392.0 \
  {mprj_pads.area1_io_pad\[13\]}
place_pad \
  -master sky130_ef_io__vccd_lvc_pad \
  -row IO_EAST \
  -location 4613.0 \
  {user1_vccd_lvclamp_pad}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_EAST \
  -location 4838.0 \
  {mprj_pads.area1_io_pad\[14\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 3130.0 \
  {mprj_pads.area1_io_pad\[15\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 2621.0 \
  {mprj_pads.area1_io_pad\[16\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 2364.0 \
  {mprj_pads.area1_io_pad\[17\]}
place_pad \
  -master sky130_ef_io__vssa_hvc_pad \
  -row IO_NORTH \
  -location 2878.0 \
  {user1_vssa_hvclamp_pad\[0\]}
place_pad \
  -master sky130_ef_io__disconnect_vccd_slice_5um \
  -row IO_NORTH \
  -location 2181.0 \
  {brk_vccd_1}
place_pad \
  -master sky130_ef_io__disconnect_vdda_slice_5um \
  -row IO_NORTH \
  -location 2176.0 \
  {brk_vdda_1}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 1919.0 \
  {mprj_pads.area2_io_pad\[0\]}
place_pad \
  -master sky130_ef_io__vssio_hvc_pad \
  -row IO_NORTH \
  -location 1667.0 \
  {mgmt_vssio_hvclamp_pad\[1\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 1410.0 \
  {mprj_pads.area2_io_pad\[1\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 1152.0 \
  {mprj_pads.area2_io_pad\[2\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 895.0 \
  {mprj_pads.area2_io_pad\[3\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 638.0 \
  {mprj_pads.area2_io_pad\[4\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_NORTH \
  -location 381.0 \
  {mprj_pads.area2_io_pad\[5\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 4771.0 \
  {mprj_pads.area2_io_pad\[6\]}
place_pad \
  -master sky130_ef_io__vccd_lvc_pad \
  -row IO_WEST \
  -location 4560.0 \
  {user2_vccd_lvclamp_pad}
place_pad \
  -master sky130_ef_io__vddio_hvc_pad \
  -row IO_WEST \
  -location 4349.0 \
  {mgmt_vddio_hvclamp_pad\[1\]}
place_pad \
  -master sky130_ef_io__vssa_hvc_pad \
  -row IO_WEST \
  -location 4138.0 \
  {user2_vssa_hvclamp_pad}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 3922.0 \
  {mprj_pads.area2_io_pad\[7\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 3706.0 \
  {mprj_pads.area2_io_pad\[8\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 3490.0 \
  {mprj_pads.area2_io_pad\[9\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 3274.0 \
  {mprj_pads.area2_io_pad\[10\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 3058.0 \
  {mprj_pads.area2_io_pad\[11\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 2842.0 \
  {mprj_pads.area2_io_pad\[12\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 2626.0 \
  {mprj_pads.area2_io_pad\[13\]}
place_pad \
  -master sky130_ef_io__vdda_hvc_pad \
  -row IO_WEST \
  -location 2415.0 \
  {user2_vdda_hvclamp_pad}
place_pad \
  -master sky130_ef_io__vssd_lvc_pad \
  -row IO_WEST \
  -location 2204.0 \
  {user2_vssd_lvclmap_pad}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 1988.0 \
  {mprj_pads.area2_io_pad\[14\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 1772.0 \
  {mprj_pads.area2_io_pad\[15\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 1556.0 \
  {mprj_pads.area2_io_pad\[16\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 1340.0 \
  {mprj_pads.area2_io_pad\[17\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 1124.0 \
  {mprj_pads.area2_io_pad\[18\]}
place_pad \
  -master sky130_ef_io__gpiov2_pad_wrapped \
  -row IO_WEST \
  -location 908.0 \
  {mprj_pads.area2_io_pad\[19\]}
place_pad \
  -master sky130_ef_io__disconnect_vccd_slice_5um \
  -row IO_WEST \
  -location 767.0 \
  {brk_vccd_2}
place_pad \
  -master sky130_ef_io__disconnect_vdda_slice_5um \
  -row IO_WEST \
  -location 762.0 \
  {brk_vdda_2}
place_pad \
  -master sky130_ef_io__vddio_hvc_pad \
  -row IO_WEST \
  -location 551.0 \
  {mgmt_vddio_hvclamp_pad\[0\]}
place_pad \
  -master sky130_ef_io__vccd_lvc_pad \
  -row IO_WEST \
  -location 340.0 \
  {mgmt_vccd_lvclamp_pad}

place_corners sky130_ef_io__corner_pad

place_io_fill \
  -row IO_NORTH \
  sky130_ef_io__com_bus_slice_20um \
  sky130_ef_io__com_bus_slice_10um \
  sky130_ef_io__com_bus_slice_5um \
  sky130_ef_io__com_bus_slice_1um
place_io_fill \
  -row IO_SOUTH \
  sky130_ef_io__com_bus_slice_20um \
  sky130_ef_io__com_bus_slice_10um \
  sky130_ef_io__com_bus_slice_5um \
  sky130_ef_io__com_bus_slice_1um
place_io_fill \
  -row IO_WEST \
  sky130_ef_io__com_bus_slice_20um \
  sky130_ef_io__com_bus_slice_10um \
  sky130_ef_io__com_bus_slice_5um \
  sky130_ef_io__com_bus_slice_1um
place_io_fill \
  -row IO_EAST \
  sky130_ef_io__com_bus_slice_20um \
  sky130_ef_io__com_bus_slice_10um \
  sky130_ef_io__com_bus_slice_5um \
  sky130_ef_io__com_bus_slice_1um

connect_by_abutment
place_io_terminals */PAD

set def_file [make_result_file "skywater130_caravel.def"]
write_def $def_file
diff_files $def_file "skywater130_caravel.defok"
