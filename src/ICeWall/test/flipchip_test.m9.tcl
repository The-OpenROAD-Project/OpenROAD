source ../src/ICeWall.tcl
source "helpers.tcl"

read_lef NangateOpenCellLibrary.m9.lef
read_lef flipchip_test.dummy_pads.m9.lef

read_liberty dummy_pads.lib

read_verilog flipchip_test/flipchip_test.v

link_design test

if {[catch {ICeWall load_footprint flipchip_test/flipchip_test.package.m9.strategy} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

initialize_floorplan \
  -die_area  {0 0 4000.000 4000.000} \
  -core_area {180.012 180.096 3819.964 3819.712} \
  -site FreePDK45_38x28_10R_NP_162NW_34O

make_tracks

#source ../../../test/Nangate45/Nangate45.tracks

if {[catch {ICeWall init_footprint flipchip_test/flipchip_test.sigmap} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

set def_file [make_result_file "flipchip.m9.def"]
set def1_file [make_result_file "flipchip_test.m9.def"]

write_def $def_file
exec sed -e "/END SPECIALNETS/r[ICeWall::get_footprint_rdl_cover_file_name]" $def_file > $def1_file

diff_files $def1_file "flipchip_test.m9.defok"
