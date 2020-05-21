# buffer_ports hands off special nets
source helpers.tcl
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def buffer_ports6.def
buffer_ports -buffer_cell BUF_X1
