# Tests the eco undo mechanism
source "helpers.tcl"

set db [ord::get_db]
read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
set chip [odb::dbChip_create $db]
set tech [$db getTech]

set block [odb::dbBlock_create $chip top]
set master1 [$db findMaster INV_X1]
set master2 [$db findMaster INV_X2]

# Setup objects
set pre_inst [odb::dbInst_create $block $master1 pre_inst]
set pre_swap [odb::dbInst_create $block $master1 pre_swap]
set pre_net [odb::dbNet_create $block pre_net]
set pre_bterm [odb::dbBTerm_create $pre_net pre_bterm]
set pre_inst_A [$pre_inst findITerm A]
$pre_inst_A connect $pre_net

# Enables internal checking
set_debug_level ODB journal_check 1

odb::dbDatabase_beginEco $block

# Create objects that will be destroyed by undo
set eco_inst [odb::dbInst_create $block $master1 eco_inst]
set eco_net [odb::dbNet_create $block eco_net]

# Changes to be undone
$pre_swap swapMaster $master2
$pre_bterm connect $eco_net

# Destroy objects that will be recreated by undo
odb::dbInst_destroy $pre_inst
odb::dbNet_destroy $pre_net

odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block

set block [$chip getBlock]
set out_def [make_result_file "undo.def"]
write_def $out_def

diff_files $out_def "undo.defok"

exit 0
