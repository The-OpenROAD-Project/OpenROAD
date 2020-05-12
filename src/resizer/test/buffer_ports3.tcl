# buffer_ports lef macro bus pins
read_liberty Nangate_typ.lib
read_liberty bus1.lib
read_lef Nangate.lef
read_lef bus1.lef
read_def bus1.def

buffer_ports -buffer_cell BUF_X2
