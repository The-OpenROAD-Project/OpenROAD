# repair_design max_cap unconstrained
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
set def_file [file join $result_dir "repair_cap2.def"]
write_hi_fanout_def $def_file 60
read_def $def_file

set_wire_rc -layer metal3
estimate_parasitics -placement

# flute results are unstable across platforms so just check
# that the violation goes away
with_output_to_variable violations { report_check_types -max_cap -violators }
puts "Found [regexp -all VIOLATED $violations] violations"

repair_design -buffer_cell BUF_X2

with_output_to_variable violations { report_check_types -max_cap -violators }
puts "Found [regexp -all VIOLATED $violations] violations"
