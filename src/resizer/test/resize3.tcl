# resize -buffer_cell arg checks
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg1.def
resize -buffer_cell xxx/yyy
resize -buffer_cell liberty1/yyy
resize -buffer_cell [get_lib_cell xxx/yyy]
resize -buffer_cell liberty1/snl_nor02x1
resize -buffer_cell liberty1/snl_bufx2
resize -buffer_cell [get_lib_cell liberty1/snl_bufx2]
