source "helpers.tcl"

read_liberty ./Nangate45/Nangate45_typ.lib
read_lef ./Nangate45/Nangate45.lef
read_lef ./Nangate45/Nangate45_stdcell.lef
read_verilog ./aes_nangate45.v
link_design aes_cipher_top
read_sdc ./aes_nangate45.sdc


puts "-- Before --\n"
report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

puts "-- After --\n"

resynth
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns

resynth
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns
