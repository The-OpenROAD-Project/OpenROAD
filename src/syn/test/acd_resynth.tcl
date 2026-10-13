read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog gcd.v
link_design gcd

create_clock -period 0.52 [get_ports clk]

puts "=== initial ==="
report_worst_slack
report_tns

puts "=== pass 1 ==="
syn::acd_resynth -apply -effort 1e13
report_worst_slack
report_tns

puts "=== pass 2 ==="
syn::acd_resynth -apply -effort 1e13
report_worst_slack
report_tns
