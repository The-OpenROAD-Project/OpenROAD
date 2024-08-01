source "helpers.tcl"

set db [ord::get_db]
read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
set chip [odb::dbChip_create $db]
set tech [$db getTech]

set block [odb::dbBlock_create $chip top]
set master [$db findMaster INV_X1]

set pre_inst [odb::dbInst_create $block $master pre_inst]
set pre_net [odb::dbNet_create $block pre_net]

# Enables internal checking
set_debug_level ODB journal_check 1

odb::dbDatabase_beginEco $block

# Create objects that will be destroyed by undo
set eco_inst [odb::dbInst_create $block $master eco_inst]
set eco_net [odb::dbNet_create $block eco_net]

# Destroy objects that will be recreated by undo
odb::dbInst_destroy $pre_inst
odb::dbNet_destroy $pre_net

odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block

foreach inst [$block getInsts] {
  puts [$inst getName]
}
foreach net [$block getNets] {
  puts [$net getName]
}

exit 0
