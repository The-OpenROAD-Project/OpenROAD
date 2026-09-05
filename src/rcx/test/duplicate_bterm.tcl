source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def duplicate_bterm.def

define_process_corner -ext_model_index 0 X
set_extraction_rules_file 45_patterns.rules
extract_parasitics -max_res 0 -coupling_threshold 0.1

set spef_file [make_result_file duplicate_bterm.spef]
write_spef $spef_file
set fp [open $spef_file r]
set spef [read $fp]
close $fp
set res_start [string first "*RES\n" $spef]
set res_end [string first "*END" $spef $res_start]
set res_section [string range $spef $res_start $res_end]
set resistor_count [regexp -all -line {^[0-9]+ [^ \t]+ [^ \t]+ [^ \t]+} $res_section]
puts "duplicate bterm resistors: $resistor_count"
if {$resistor_count != 3} {
  error "expected three resistors for the three physical wire segments"
}
