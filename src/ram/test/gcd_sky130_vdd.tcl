source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def sky130hd_data/gcd_sky130hd_floorplan.def
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_sdc sky130hd_data/gcd_sky130hd_floorplan.sdc

check_power_grid -net VDD
