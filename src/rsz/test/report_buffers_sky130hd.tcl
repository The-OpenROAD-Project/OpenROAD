# report_buffers test for sky130hd library

source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_def repair_design2.def

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true

report_buffers -filtered
rsz::report_fast_buffer_sizes
