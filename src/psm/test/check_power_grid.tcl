# Check that check_power_grid works without a liberty file
# or the voltage on the net being set
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
check_power_grid -net VDD
