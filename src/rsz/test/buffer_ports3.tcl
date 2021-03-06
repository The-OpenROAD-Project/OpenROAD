# buffer_ports lef macro bus pins
read_liberty Nangate45/Nangate45_typ.lib
read_liberty bus1.lib
read_lef Nangate45/Nangate45.lef
read_lef bus1.lef
read_def bus1.def

buffer_ports
