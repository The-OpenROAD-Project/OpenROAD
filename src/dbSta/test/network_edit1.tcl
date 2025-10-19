# connect/disconnect pin/port
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg3.def

disconnect_pin in1 r1/D
disconnect_pin in1 [get_ports in1]
report_net in1

set def_file [make_result_file "network_edit1.def"]
write_def $def_file
diff_files $def_file "network_edit1.defok"

odb::dbChip_destroy [odb::dbDatabase_getChip [ord::get_db]]
read_def $def_file

connect_pin in1 r1/D
connect_pin in1 [get_ports in1]
report_net in1
