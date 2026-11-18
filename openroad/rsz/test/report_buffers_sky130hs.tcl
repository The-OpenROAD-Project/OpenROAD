# report_buffers test for sky130hs library

source "helpers.tcl"
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def repair_hold4.def

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true

report_buffers -filtered
rsz::report_fast_buffer_sizes
