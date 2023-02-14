source "helpers.tcl"

read_lef ../../../test/sky130hd/sky130hd.tlef
read_lef ../../../test/sky130hd/sky130_fd_sc_hd_merged.lef
read_lef caravel_sky130/lef/sky130_ef_io__com_bus_slice_10um.lef
read_lef caravel_sky130/lef/sky130_ef_io__com_bus_slice_1um.lef
read_lef caravel_sky130/lef/sky130_ef_io__com_bus_slice_20um.lef
read_lef caravel_sky130/lef/sky130_ef_io__com_bus_slice_5um.lef
read_lef caravel_sky130/lef/sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um.lef
read_lef caravel_sky130/lef/sky130_ef_io__corner_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__disconnect_vccd_slice_5um.lef
read_lef caravel_sky130/lef/sky130_ef_io__disconnect_vdda_slice_5um.lef
read_lef caravel_sky130/lef/sky130_ef_io__gpiov2_pad_wrapped.lef
read_lef caravel_sky130/lef/sky130_ef_io__vccd_hvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vccd_lvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vdda_hvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vdda_lvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vddio_hvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vddio_lvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vssa_hvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vssa_lvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vssd_hvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vssd_lvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vssio_hvc_pad.lef
read_lef caravel_sky130/lef/sky130_ef_io__vssio_lvc_pad.lef
read_lef caravel_sky130/lef/sky130_fd_io__top_xres4v2.lef

read_liberty caravel_sky130/lib/sky130_dummy_io.lib  
read_liberty ../../../test/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_def caravel_sky130/chip_io.def

source caravel_sky130/library.sky130_fd_io.tcl

read_verilog sub_modules_sky130.v
link_design sub_modules

add_global_connection -net {VPWR} -pin_pattern {^VPWR$} -power
add_global_connection -net {VGND} -pin_pattern {^VGND$} -ground
global_connect

insert_dft
write_verilog result_sub_modules_sky130.v
diff_files result_sub_modules_sky130.v result_sub_modules_sky130.vok
