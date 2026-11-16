source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_slow.lib
read_liberty Nangate45/Nangate45_fast.lib
define_scene min -liberty Nangate45/Nangate45_fast.lib
define_scene max -liberty Nangate45/Nangate45_slow.lib
set_scene min
read_sdc Nangate45_data/gcd.sdc

source Nangate45_data/Nangate45_corners.rc

analyze_power_grid -scene min -vsrc Vsrc_gcd_vdd.loc -net VDD

analyze_power_grid -scene max -vsrc Vsrc_gcd_vdd.loc -net VDD
