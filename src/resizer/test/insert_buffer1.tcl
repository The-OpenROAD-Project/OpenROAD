# def buffer upsize, insertion
source helpers.tcl
source read_reg1.tcl
# buffer upsize
replace_cell u1 snl_bufx2
report_checks -fields input_pin

# buffer insertion
disconnect_pin u2z r3/D
make_net b1z
make_instance b1 snl_bufx1
connect_pin u2z b1/A
connect_pin b1z b1/Z
connect_pin b1z r3/D
report_checks -fields input_pin

set def_file [make_result_file insert_buffer1.def]
write_def -sort $def_file
report_file $def_file
