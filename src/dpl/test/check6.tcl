source "helpers.tcl"
# std cell abutting block
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def check6.def
set_debug_level DPL detailed 1 
detailed_placement
check_placement -verbose
