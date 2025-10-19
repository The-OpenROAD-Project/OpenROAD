# manual buffer removal test
# in1 -> b1 -> b2 -> b3 -> out1
# remove buffers b1 and b3 only
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers3.def

# make sure sta works before/after removal
report_checks -unconstrained
set_dont_touch b1
remove_buffers b1 b3
report_checks -unconstrained

set repaired_filename [make_result_file "remove_buffers3.def"]
write_def $repaired_filename
diff_file remove_buffers3.defok $repaired_filename
