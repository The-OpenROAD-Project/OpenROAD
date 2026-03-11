# Test grt::update_net with CUGR incremental routing.
# Design (remove_buffers1.def): in1 -> b3 -> b2 -> b1 -> b4 -> out1
# We split net n2 (b2/Z -> b1/A) by disconnecting both iterms and
# reconnecting them to a brand-new net n_bypass.
# After calling grt::update_net on both nets and completing the incremental
# route, n_bypass must appear in the guide file.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

global_route -verbose -use_cugr

# --- begin topology mutation ---
global_route -start_incremental

set block [ord::get_db_block]
set b1 [$block findInst "b1"]
set b2 [$block findInst "b2"]
set n2_net [$block findNet "n2"]

# Create the bypass net (will carry b2/Z -> b1/A, currently on n2)
set n_bypass_net [odb::dbNet_create $block "n_bypass"]

# Detach b2/Z and b1/A from n2
set b2_z [$b2 findITerm "Z"]
set b1_a [$b1 findITerm "A"]
$b2_z disconnect
$b1_a disconnect

# Attach them to the new net
odb::dbITerm_connect $b2_z $n_bypass_net
odb::dbITerm_connect $b1_a $n_bypass_net

# Refresh CUGR state:
#   n2      now has 0 pins  -> will be skipped during routing
#   n_bypass now has 2 pins -> will be routed
grt::update_cugr_net $n2_net
grt::update_cugr_net $n_bypass_net

global_route -end_incremental
# --- end topology mutation ---

set guide_file [make_result_file "incremental_update_net.guide"]
write_guides $guide_file

check "n_bypass net has routing guides" {
  set fh [open $guide_file r]
  set content [read $fh]
  close $fh
  expr { [string first "n_bypass" $content] >= 0 }
} 1

exit_summary
