source "helpers.tcl"
source ../src/ICeWall.tcl

read_lef NangateOpenCellLibrary.mod.lef
read_lef dummy_pads.lef

read_liberty dummy_pads.lib

read_verilog soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.v

link_design soc_bsg_black_parrot

if {[catch {ICeWall load_footprint soc_bsg_black_parrot_nangate45/bsg_black_parrot.package.strategy} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

initialize_floorplan \
  -die_area  [ICeWall get_die_area] \
  -core_area [ICeWall get_core_area] \
  -tracks    [ICeWall get_tracks] \
  -site      FreePDK45_38x28_10R_NP_162NW_34O

if {[catch {ICeWall init_footprint soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.sigmap} msg]} {
  puts $errorInfo
  puts $msg
  exit
}

set def_file results/soc_bsg_black_parrot_nangate45.init.def

write_def $def_file
diff_files $def_file soc_bsg_black_parrot_nangate45.defok
