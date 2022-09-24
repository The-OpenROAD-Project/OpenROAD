source "helpers.tcl"
read_lef sky130_data/sky130hd.tlef
read_lef  sky130_data/sky130hd_std_cell.lef 
read_def sky130_data/gcd_sky130hd_floorplan.def
read_liberty sky130_data/sky130hd_tt.lib
read_sdc sky130_data/gcd_sky130hd_floorplan.sdc

check_power_grid -net VDD 
