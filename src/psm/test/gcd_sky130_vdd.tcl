read_lef sky130_data/lef/sky130_fd_sc_hd.tlef  
read_lef  sky130_data/lef/sky130_fd_sc_hd_merged.lef  
read_lef sky130_data/lef/sky130io_fill.lef
read_def sky130_data/gcd_sky130hd_floorplan.def
read_liberty sky130_data/lib/sky130_fd_sc_hd__tt_025C_1v80.lib
read_liberty sky130_data/lib/sky130_dummy_io.lib  
read_sdc sky130_data/gcd_sky130hd_floorplan.sdc

check_power_grid -net VDD 
