# repair_max_cap hi fanout register array
source "helpers.tcl"
source "hi_fanout.tcl"
source "report_max_cap.tcl"

read_liberty liberty1.lib
read_lef liberty1.lef
set def_file [file join $result_dir "hi_fanout.def"]
write_hi_fanout_def $def_file 40
read_def $def_file
create_clock -period 1 clk1

# kohm/micron, pf/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 1.3e-2
report_max_cap 5

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
repair_max_cap -buffer_cell $buffer_cell
#report_max_cap 5
