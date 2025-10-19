source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/aes.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/aes.sdc

analyze_power_grid -vsrc Vsrc_aes_vdd.loc -net VDD
