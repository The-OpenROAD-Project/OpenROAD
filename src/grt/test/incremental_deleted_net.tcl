# Test deleted net handling in CUGR incremental routing.
# a. Read a small LEF/DEF
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal metal2-metal8 -clock metal3-metal8

# b. Run global_route with -router cugr to get a baseline full route
global_route -verbose -use_cugr

# c. Call global_route -start_incremental
global_route -start_incremental -use_cugr

set block [ord::get_db_block]
set n2_net [$block findNet "n2"]

# d. Insert a buffer manually using ODB API (creates intermediate net n_extra)
set n_extra_net [odb::dbNet_create $block "n_extra"]
set db [ord::get_db]
set master [$db findMaster "BUF_X1"]
if { $master == "NULL" } {
  utl::error GRT 9999 "Failed to find BUF_X1 master"
}
set b_extra [odb::dbInst_create $block $master "b_extra"]

# Disconnect b1/A from n2 and connect to n_extra (b_extra output)
set b1 [$block findInst "b1"]
set b1_a [$b1 findITerm "A"]
$b1_a disconnect
odb::dbITerm_connect $b1_a $n_extra_net

# Connect b_extra: input from n2, output to n_extra
set b_extra_a [$b_extra findITerm "A"]
set b_extra_z [$b_extra findITerm "Z"]
odb::dbITerm_connect $b_extra_a $n2_net
odb::dbITerm_connect $b_extra_z $n_extra_net

# e. Remove that buffer so the intermediate net is deleted
$b1_a disconnect
odb::dbITerm_connect $b1_a $n2_net ;# reconnect b1/A back to original n2
odb::dbInst_destroy $b_extra
odb::dbNet_destroy $n_extra_net

# f. Mark the affected nets dirty
grt::add_dirty_net $n2_net

# g. Call global_route -end_incremental
global_route -end_incremental -use_cugr

# h. Write guides to a result file
set guide_file [make_result_file "incremental_deleted_net.guide"]
write_guides $guide_file

# i. Assert the deleted intermediate net name does NOT appear in the guide file
check "n_extra net (deleted) does NOT have routing guides" {
  set fh [open $guide_file r]
  set content [read $fh]
  close $fh
  expr { [string first "n_extra" $content] == -1 }
} 1

exit_summary
