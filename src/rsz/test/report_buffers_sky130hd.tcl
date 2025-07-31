# report_buffers test for sky130hd library
# with modified LEF

source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_def repair_design2.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

report_buffers -filtered
rsz::report_fast_buffer_sizes
