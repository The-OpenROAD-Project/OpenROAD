# Check that check_power_grid works with a disconnected macro,
# which is not logically connected to the power grid
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_lef Nangate45_data/fakeram45_64x32.lef

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x32.lib

read_def Nangate45_data/check_power_grid_ok_disconnected.def
check_power_grid -net VDD
