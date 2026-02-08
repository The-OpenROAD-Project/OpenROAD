source "helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p50V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef

read_verilog gcd_ihp.v
link gcd

report_opt_config

report_buffers -filtered
rsz::report_fast_buffer_sizes

set_opt_config -disable_buffer_pruning true
report_opt_config

report_buffers -filtered
rsz::report_fast_buffer_sizes

puts "\n==== set clock buffer footprint to DLY ==="
set_cts_config -clock_buffer_footprint DLY
report_buffers

reset_cts_config -clock_buffer_footprint
puts "\n==== set clock buffer string to dlygate ==="
set_cts_config -clock_buffer_string dlygate
report_buffers

puts "\n==== set clock buffer footprint to DLY ==="
puts "==== should get an error ==="
set_cts_config -clock_buffer_footprint DLY
