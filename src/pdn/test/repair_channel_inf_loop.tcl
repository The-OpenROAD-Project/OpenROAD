# Check for repair channels with straps in same direction and reject
# this avoids the infinate loop reported in: https://github.com/The-OpenROAD-Project/OpenROAD/issues/5905

read_lef repair_channel_inf_loop/tech_macro.lef
read_def repair_channel_inf_loop/floorplan.def

set_voltage_domain -power VDD -ground GND

define_pdn_grid -name "Core"

add_pdn_stripe -followpins -layer M1 -width 2.630

add_pdn_stripe -layer M3 -width 11.280 -pitch 225.600 -spacing 16.920 -offset 113.550
add_pdn_stripe -layer M4 -width 11.280 -pitch 526.400 -spacing 16.920 -offset 375.625

add_pdn_connect -layers {M3 M4}
add_pdn_connect -layers {M1 M3}

catch {pdngen} err
puts $err
