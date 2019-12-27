# connect/disconnect pin/port
source "helpers.tcl"
read_lef example1.lef
read_def example1.def
read_liberty example1_slow.lib

disconnect_pin in1 r1/D
disconnect_pin in1 [get_ports in1]
report_net -connections in1

set def_file [make_result_file "network_edit1.def"]
write_def $def_file
diff_file $def_file "network_edit1.defok"

odb::dbChip_destroy [odb::dbDatabase_getChip [ord::get_db]]
read_def $def_file

set db_file [make_result_file "network_edit1.db"]
write_db $db_file

connect_pin in1 r1/D
connect_pin in1 [get_ports in1]
report_net -connections in1
