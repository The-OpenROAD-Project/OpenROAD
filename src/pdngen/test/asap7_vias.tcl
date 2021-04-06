source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_201209.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x_201211.lef
read_def asap7_vias/2_5_floorplan_tapcell.def

pdngen asap7_vias/grid_strategy-M2-M4-M7.cfg -verbose

set def_file results/asap7_vias.def
write_def $def_file 

diff_files $def_file asap7_vias.defok
