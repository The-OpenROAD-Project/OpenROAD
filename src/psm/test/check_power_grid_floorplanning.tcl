# Check that check_power_grid works without with floorplanning flag
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd_floorplanning.def
check_power_grid -net VDD -floorplanning
