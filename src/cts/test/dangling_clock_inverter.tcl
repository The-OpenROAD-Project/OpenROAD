# Minimal repro for https://github.com/The-OpenROAD-Project/OpenROAD/issues/11349
#
# A clock inverter directly sinking off the top clock net, whose output net
# has zero downstream signal-input load (fully dangling) -- reproduces the
# null dereference in TritonCTS::separateMacroRegSinks() without needing a
# full placed design or repair_clock_inverters.

source "helpers.tcl"
source Nangate45/Nangate45.vars

read_lib $liberty_file
read_lef $tech_lef
read_lef $std_cell_lef

set db [ord::get_db]
set tech [ord::get_db_tech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip "top"]
$block setDefUnits 2000

set clk [odb::dbNet_create $block "clk"]
$clk setSigType CLOCK
set clk_bterm [odb::dbBTerm_create $clk "clk"]
$clk_bterm setSigType CLOCK
$clk_bterm setIoType INPUT
set clk_bpin [odb::dbBPin_create $clk_bterm]
$clk_bpin setPlacementStatus PLACED
set layer [$tech findLayer "metal5"]
odb::dbBox_create $clk_bpin $layer 0 0 2000 2000

# A couple of real register sinks so CTS has something to build a tree for.
set dff [$db findMaster "DFF_X1"]
for { set i 0 } { $i < 4 } { incr i } {
  set inst [odb::dbInst_create $block $dff "dff_${i}"]
  $inst setLocation [expr { $i * 40000 }] 0
  $inst setPlacementStatus PLACED
  set ck [$inst findITerm "CK"]
  $ck connect $clk
}

# The dangling clock inverter: input tied directly to clk, output net has
# NO signal-input load at all (nothing else connected to it).
set inv [$db findMaster "INV_X1"]
set inv_inst [odb::dbInst_create $block $inv "dangling_inv"]
$inv_inst setLocation 200000 0
$inv_inst setPlacementStatus PLACED
set a_iterm [$inv_inst findITerm "A"]
$a_iterm connect $clk

set dangling_net [odb::dbNet_create $block "dangling_net"]
set zn_iterm [$inv_inst findITerm "ZN"]
$zn_iterm connect $dangling_net
# dangling_net intentionally left with no other iterm.

ord::design_created

initialize_floorplan -die_area { 0 0 400 400 } -core_area { 0 0 400 400 } -site $site
source $tracks_file
source Nangate45/Nangate45.rc

create_clock -period 5 clk
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

clock_tree_synthesis -sink_clustering_enable
puts "pass"
