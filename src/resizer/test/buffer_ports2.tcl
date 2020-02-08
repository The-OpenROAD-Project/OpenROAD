# -buffer_cell arg checks
source "helpers.tcl"
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg1.def

buffer_ports -buffer_cell xxx/yyy
buffer_ports -buffer_cell liberty1/yyy
buffer_ports -buffer_cell [get_lib_cell xxx/yyy]
buffer_ports -buffer_cell liberty1/snl_nor02x1
buffer_ports -buffer_cell liberty1/snl_bufx2
buffer_ports -buffer_cell [get_lib_cell liberty1/snl_bufx2]
