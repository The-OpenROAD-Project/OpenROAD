source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/aes_multi_bterms.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/aes.sdc

analyze_power_grid -net VDD
analyze_power_grid -net VSS
