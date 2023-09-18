source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_def asap7_data/aes_place.def
read_liberty asap7_data/asap7.lib
read_sdc asap7_data/aes_place.sdc

source asap7/setRC.tcl

analyze_power_grid -net VDD
