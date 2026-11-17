source "helpers.tcl"
# floorplan corner missing rows requires -max_displacement to pull in pin buffers
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_1024x32.lef

read_def max_disp1.def

detailed_placement -max_displacement {280 50}
check_placement -verbose
