# check_antennas after incremental CUGR routing: a rebuffered sink makes a
# local net whose guides the incremental path must rebuild, or antenna
# checking fails the whole design with ANT-0008.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route -use_cugr

set block [ord::get_db_block]
set sink [$block findITerm "_667_/D"]
set target [$sink getNet]
lassign [[$sink getInst] getLocation] x y

# Rebuffer the sink at its own location; the new net's pins share a gcell.
global_route -start_incremental
set buf [odb::dbInst_create $block \
  [[ord::get_db] findMaster "sky130_fd_sc_hs__buf_1"] "local_buf"]
$buf setLocation $x $y
$buf setPlacementStatus PLACED
set buf_net [odb::dbNet_create $block "local_buf_net"]
$sink disconnect
$sink connect $buf_net
[$buf findITerm "A"] connect $target
[$buf findITerm "X"] connect $buf_net
global_route -end_incremental

puts "local net guides: [llength [$buf_net getGuides]]"
check_antennas
