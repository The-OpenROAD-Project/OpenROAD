# floorplan corner missing rows requires -max_displacement to pull in pin buffers
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_1024x32.lef

read_liberty Nangate45/Nangate45_fast.lib
read_liberty Nangate45/fakeram45_1024x32.lib

read_def max_disp1.def

detailed_placement -max_displacement {280 50}
check_placement -verbose
