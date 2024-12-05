# filler_placement with set_placement_padding with verbose
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def simple01.def
set_placement_padding -global -left 2 -right 2

detailed_placement
filler_placement FILL* -verbose
