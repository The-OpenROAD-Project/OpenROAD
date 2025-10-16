# remove_buffers in1 -> inv -> b1 -> out1
#                           -> b2 -> out2
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers2.def

# make sure sta works before/after removal
report_checks -unconstrained
remove_buffers
report_checks -unconstrained

set repaired_filename [make_result_file "remove_buffers2.def"]
write_def $repaired_filename
diff_file remove_buffers2.defok $repaired_filename
