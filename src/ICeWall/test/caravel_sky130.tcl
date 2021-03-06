source "helpers.tcl"

read_lef caravel_sky130/lef/sky130_fd_pr.tlef
read_lef caravel_sky130/lef/sky130_fd_sc_hd_merged.lef
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
read_liberty caravel_sky130/lib/sky130_fd_sc_hd__tt_025C_1v80.lib

read_def caravel_sky130/chip_io.def

puts "Loading library data"
source caravel_sky130/library.sky130_fd_io.tcl

puts "Extracting footprint"

ICeWall extract_footprint
ICeWall write_footprint results/caravel_sky130.package.tcl

set env(FOOTPRINT_LIBRARY) caravel_sky130/library.sky130_fd_io.tcl
diff_files results/caravel_sky130.package.tcl caravel_sky130.package.tclok

puts "Checking result"
read_verilog caravel_sky130/chip_io.v

link_design chip_io

if {[catch {ICeWall load_footprint results/caravel_sky130.package.tcl} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

initialize_floorplan \
  -die_area  [ICeWall get_die_area] \
  -core_area [ICeWall get_core_area] \
  -site      unithd

if {[catch {ICeWall init_footprint caravel_sky130/chip_io.sigmap} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

set def_file results/caravel_sky130.def

write_def $def_file
diff_files $def_file caravel_sky130.defok

