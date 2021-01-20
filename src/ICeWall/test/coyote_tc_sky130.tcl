source "helpers.tcl"
source ../src/ICeWall.tcl

read_lef ../../../test/sky130/sky130_tech.lef
read_lef ../../../test/sky130/sky130_std_cell.lef

foreach file [glob -nocomplain coyote_tc/lef/*] {
  puts $file
  read_lef $file
}

read_liberty ../../../test/sky130/sky130_tt.lib
foreach file [glob -nocomplain coyote_tc/lib/*] {
  read_liberty $file
}

read_verilog coyote_tc/1_synth.v

link_design coyote_tc

if {[catch {ICeWall load_footprint coyote_tc/coyote_tc.package.strategy} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

initialize_floorplan \
  -die_area  {0 0 5400.000 4616.000} \
  -core_area {200.0 200.0 5200.0 4416.0} \
  -tracks    [ICeWall get_tracks] \
  -site      unit

if {[catch {ICeWall init_footprint coyote_tc/coyote_tc.sigmap} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

set def_file results/coyote_tc_sky130.def

write_def $def_file
diff_files $def_file coyote_tc_sky130.defok
