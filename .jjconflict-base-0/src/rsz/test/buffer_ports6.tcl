# buffer_ports hands off special nets
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def buffer_ports6.def
buffer_ports
