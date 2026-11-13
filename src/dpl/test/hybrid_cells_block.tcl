# Movable block macro without a SITE on hybrid rows
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def hybrid_cells.def
set inst [odb::dbInst_create [ord::get_db_block] \
  [[ord::get_db] findMaster HM_100x100_1x1] b1]
$inst setLocation 400000 400000
$inst setPlacementStatus PLACED
detailed_placement
check_placement -verbose
