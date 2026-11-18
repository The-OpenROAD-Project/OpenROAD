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

set_pdnsim_inst_power -corner min -inst _440_ -power 1e-5
set_pdnsim_inst_power -corner min -inst _829_ -power 1e-5
set_pdnsim_inst_power -corner max -inst _440_ -power 1e-4
set_pdnsim_inst_power -corner max -inst _829_ -power 1e-4

analyze_power_grid -corner min -vsrc Vsrc_gcd_vdd.loc -net VDD

analyze_power_grid -corner max -vsrc Vsrc_gcd_vdd.loc -net VDD
