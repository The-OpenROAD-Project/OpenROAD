# Exercise dbSdcNetwork::findInstancesMatching1 prefix pruning via a toy SDC.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top -hier
read_sdc sdc_hier_wildcard.sdc

puts "--- b1/* ---"
report_object_full_names [get_cells b1/*]
puts "--- b2/u* ---"
report_object_full_names [get_cells b2/u*]
puts "--- no_such/* (prefix miss -- expect empty) ---"
set empty [get_cells -quiet no_such/*]
puts "count=[llength $empty]"
puts "--- */r1 (leading wildcard, full DFS) ---"
report_object_full_names [get_cells */r1]
puts "--- b1/r1 (fully literal) ---"
report_object_full_names [get_cells b1/r1]
