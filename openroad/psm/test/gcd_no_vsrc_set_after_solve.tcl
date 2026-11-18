# Check that a voltage or power set after a solve is used by the next solve
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/gcd.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/gcd.sdc
analyze_power_grid -net VDD

# SDC voltage set after a solve
set_voltage 1.3 -object_list VDD
analyze_power_grid -net VDD

# First user voltage set after a solve
set_pdnsim_net_voltage -net VDD -voltage 1.5
analyze_power_grid -net VDD -allow_reuse

# First instance power set after a solve
set_pdnsim_inst_power -inst _440_ -power 1e-3
analyze_power_grid -net VDD -allow_reuse
