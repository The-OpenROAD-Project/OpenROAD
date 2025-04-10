source "helpers.tcl"

read_liberty ./Nangate45/Nangate45_typ.lib
read_lef ./Nangate45/Nangate45.lef
read_lef ./Nangate45/Nangate45_stdcell.lef
read_verilog ./aes_nangate45.v
link_design aes_cipher_top
read_sdc ./aes_nangate45.sdc

report_checks 

puts "-- Before --\n"
report_timing_histogram
resynth
puts "-- After --\n"
report_timing_histogram

report_checks 