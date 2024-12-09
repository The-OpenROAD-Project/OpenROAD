source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
define_corners "min" "max"
read_liberty -corner max Nangate45/Nangate45_slow.lib
read_liberty -corner min Nangate45/Nangate45_fast.lib
read_sdc Nangate45_data/gcd.sdc

source Nangate45_data/Nangate45_corners.rc

set_pdnsim_inst_power -corner min -inst _440_ -power 1e-5
set_pdnsim_inst_power -corner min -inst _829_ -power 1e-5
set_pdnsim_inst_power -corner max -inst _440_ -power 1e-4
set_pdnsim_inst_power -corner max -inst _829_ -power 1e-4

analyze_power_grid -corner min -vsrc Vsrc_gcd_vdd.loc -net VDD

analyze_power_grid -corner max -vsrc Vsrc_gcd_vdd.loc -net VDD
