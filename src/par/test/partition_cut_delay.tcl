# Verify that cut delay affects timing-aware partitioning without guardbanding.
set test_name partition_cut_delay
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog gcd.v
link_design gcd
create_clock [get_ports clk] -name core_clock -period 0.25
set_input_delay 0 -clock core_clock [delete_from_list [all_inputs] [get_ports clk]]
set_output_delay 0 -clock core_clock [all_outputs]

set solutions {}
foreach delay { 0 1e-9 } {
  set part_file [make_result_file "${test_name}_${delay}.part"]
  triton_part_design \
    -solution_file $part_file \
    -extra_delay $delay \
    -guardband_flag false
  set stream [open $part_file r]
  lappend solutions [read $stream]
  close $stream
}

if { [lindex $solutions 0] eq [lindex $solutions 1] } {
  error "Cut delay did not change the partition without guardbanding"
}
puts "pass"
