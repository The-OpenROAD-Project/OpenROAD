source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg2.def

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true

report_buffers -filtered
rsz::report_fast_buffer_sizes
