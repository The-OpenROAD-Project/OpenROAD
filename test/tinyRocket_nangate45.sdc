set_units -time ns
create_clock [get_ports clock]  -name core_clock  -period 5.6
set_max_fanout 100 [current_design]
