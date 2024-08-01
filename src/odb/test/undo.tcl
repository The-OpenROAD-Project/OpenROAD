source "helpers.tcl"

set db [ord::get_db]
read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
set chip [odb::dbChip_create $db]
set tech [$db getTech]

set block [odb::dbBlock_create $chip top]
set master1 [$db findMaster INV_X1]
set master2 [$db findMaster INV_X2]

set pre_inst [odb::dbInst_create $block $master1 pre_inst]
set pre_swap [odb::dbInst_create $block $master1 pre_swap]
set pre_net [odb::dbNet_create $block pre_net]

# Enables internal checking
set_debug_level ODB journal_check 1

odb::dbDatabase_beginEco $block

# Create objects that will be destroyed by undo
set eco_inst [odb::dbInst_create $block $master1 eco_inst]
set eco_net [odb::dbNet_create $block eco_net]
$pre_swap swapMaster $master2

# Destroy objects that will be recreated by undo
odb::dbInst_destroy $pre_inst
odb::dbNet_destroy $pre_net

odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block

foreach inst [$block getInsts] {
  puts "[$inst getName] [[$inst getMaster] getName]"
}
foreach net [$block getNets] {
  puts [$net getName]
}

exit 0
