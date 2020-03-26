# std cell abutting on the right block with
read_lef Nangate45.lef
read_lef extra.lef
read_def check6.def
set_placement_padding -global -right 1
detailed_placement
check_placement -verbose
