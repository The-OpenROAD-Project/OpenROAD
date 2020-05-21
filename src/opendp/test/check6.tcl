# std cell abutting block
read_lef Nangate45.lef
read_lef extra.lef
read_def check6.def
detailed_placement
check_placement -verbose
