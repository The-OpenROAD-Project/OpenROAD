source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef Nangate45_data/fakeram45_64x32.lef

read_def check_power_grid_macros.def

check_power_grid -net VDD
