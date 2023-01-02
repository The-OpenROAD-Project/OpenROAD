source "helpers.tcl"
read_lef asap7_data/asap7.lef
read_def asap7_data/aes_place.def
read_liberty asap7_data/asap7.lib
read_sdc asap7_data/aes_place.sdc
analyze_power_grid -net VDD
