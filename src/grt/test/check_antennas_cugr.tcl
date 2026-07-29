# check_antennas after incremental CUGR routing: a rebuffered sink makes a
# local net whose guides the incremental path must rebuild (ANT-0008), and
# repair_antennas warns and skips (GRT-0310) until CUGR repair is supported.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route -use_cugr

set block [ord::get_db_block]
set target [$block findNet "_000_"]
set sink ""
foreach iterm [$target getITerms] {
  if { [$iterm getIoType] == "INPUT" } {
    set sink $iterm
    break
  }
}
set loc [[$sink getInst] getLocation]

# Rebuffer the sink at its own location; the new net's pins share a gcell.
global_route -start_incremental
set master [[ord::get_db] findMaster "sky130_fd_sc_hs__buf_1"]
set buf [odb::dbInst_create $block $master "local_buf"]
$buf setLocation [lindex $loc 0] [lindex $loc 1]
$buf setPlacementStatus PLACED
set buf_net [odb::dbNet_create $block "local_buf_net"]
$sink disconnect
$sink connect $buf_net
foreach iterm [$buf getITerms] {
  set io [$iterm getIoType]
  if { $io == "INPUT" } {
    $iterm connect $target
  } elseif { $io == "OUTPUT" } {
    $iterm connect $buf_net
  }
}
global_route -end_incremental

puts "local net guides: [llength [$buf_net getGuides]]"
check_antennas
repair_antennas
check_antennas
