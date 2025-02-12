source "helpers.tcl"
set def_filename [make_result_file "repair_fanout7_multi.def"]
set repair_args [list -max_repairs_per_pass 10]
source "repair_fanout7.tcl"
