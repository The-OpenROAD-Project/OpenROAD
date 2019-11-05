# resize -buffer_cell arg checks
source "helpers.tcl"
read_liberty liberty1.lib
read_lef -tech -library liberty1.lef
read_def reg1.def
init_sta_db

resize -buffer_cell xxx/yyy
resize -buffer_cell liberty1/yyy
resize -buffer_cell [get_lib_cell xxx/yyy]
resize -buffer_cell liberty1/snl_nor02x1
resize -buffer_cell liberty1/snl_bufx2
resize -buffer_cell [get_lib_cell liberty1/snl_bufx2]
