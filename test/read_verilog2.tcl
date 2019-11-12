# hieararchical verilog
source "helpers.tcl"
read_lef liberty1.lef
read_verilog hier1.v
link_design top

report_object_names [get_cells b1/r1]
report_object_names [get_nets b1out]
report_instance b1/r1
report_net b1out
report_net b1/u1out
report_net b1/r1q
report_net b2/r1q

set def_file [make_result_file read_verilog2.def]
write_def $def_file
report_file $def_file
