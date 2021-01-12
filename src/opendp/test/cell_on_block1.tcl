# std cell on top of block
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_lef block2.lef
read_def cell_on_block1.def
detailed_placement
check_placement -verbose
