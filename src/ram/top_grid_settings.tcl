source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/aes.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/aes.sdc

check_power_grid -net VDD
set_pdnsim_source_settings -bump_dx 45 -bump_dy 45 -bump_size 25 -bump_interval 2
analyze_power_grid -net VDD -source_type BUMPS
set_pdnsim_source_settings -bump_dx 65 -bump_dy 65 -bump_size 25 -bump_interval 4
analyze_power_grid -net VDD -source_type BUMPS

set_pdnsim_source_settings -strap_track_pitch 5
analyze_power_grid -net VDD -source_type STRAPS

set_pdnsim_source_settings -strap_track_pitch 15
analyze_power_grid -net VDD -source_type STRAPS
