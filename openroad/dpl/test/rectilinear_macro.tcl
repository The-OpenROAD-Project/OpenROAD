# fixed L-shaped block whose notch holds placement sites
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef rectilinear_macro.lef
read_def rectilinear_macro.def

detailed_placement
check_placement
