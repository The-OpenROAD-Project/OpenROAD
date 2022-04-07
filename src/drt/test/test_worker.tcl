read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef

read_def gcd_nangate45_preroute.def

detailed_route_debug -dr
detailed_route_run_worker debug.design debug.globals debug.worker