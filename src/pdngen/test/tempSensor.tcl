source "helpers.tcl"

read_lef sky130hd/sky130_fd_sc_hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130hd/HEADER.lef
read_lef sky130hd/SLC.lef

read_def tempSensor/2_5_floorplan_tapcell.def

pdngen tempSensor/pdn.cfg -verbose

set def_file results/tempSensor.def
write_def $def_file 

diff_files $def_file tempSensor.defok
