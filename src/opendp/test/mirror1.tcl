# optimize_mirroring
read_lef Nangate45/Nangate45.lef
read_def gcd_replace.def
set_placement_padding -global -left 1 -right 1
detailed_placement
optimize_mirroring
