source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc
set_pdnsim_net_voltage -net VDD -voltage 1.5
analyze_power_grid -net VDD
analyze_power_grid -net VSS
