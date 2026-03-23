source "helpers.tcl"


read_lef "data/gscl45nm.lef"
read_def "data/design.def"

set inst [[ord::get_db_block] findInst _g0_]
set new_master [[ord::get_db] findMaster NAND2X1]
$inst swapMaster $new_master

puts "New inst master [[$inst getMaster] getName]"

puts "pass"
exit 0
