# optimize_mirroring ignore BLOCKs
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def mirror2.def
set_placement_padding -global -left 1 -right 1
detailed_placement
puts "block orient [[[ord::get_db_block] findInst block1] getOrient]"
optimize_mirroring
puts "block orient [[[ord::get_db_block] findInst block1] getOrient]"
