source "helpers.tcl"

read_lef ../../../test/sky130hd/sky130hd.tlef 
read_lef ../../../test/sky130hd/sky130_fd_sc_hd_merged.lef 

read_lef sky130hd/power_switch.lef  

read_def power_switch/2_5_floorplan_tapcell.def

pdngen power_switch/pdn.cfg -verbose

set def_file results/power_switch.def
write_def $def_file 

# diff_files $def_file power_switch.defok