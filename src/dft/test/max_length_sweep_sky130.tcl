source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog max_chain_count_sky130.v
link_design max_chain_count

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

scan_replace

set max_chains 3
for { set max_length 10 } { $max_length >= 1 } { incr max_length -1 } {
  puts "==== sweep max_length=$max_length max_chains=$max_chains ===="
  set_dft_config -max_length $max_length -max_chains $max_chains
  report_dft_plan
}

