# scan_opt -rebalance_chains with two chains per clock domain, then scan_opt
# again in a fresh process on the design read back from an odb file.
source "helpers.tcl"
source "scan_opt_helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_architect_sky130.v
link_design scan_architect

create_clock -name clock1 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock1}]
create_clock -name clock2 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock2}]

set_dft_config -max_length 3 -clock_mixing no_mix

scan_replace
scatter_insts 0
execute_dft_plan

set lasts [scan_chain_last_cells]
scatter_insts 5
scan_opt -rebalance_chains
check_scan_chains $lasts
report_chain_metrics

set db_file [make_result_file scan_opt_rebalance_sky130.odb]
write_db $db_file

# Run scan_opt again in a separate process, on the design read back from
# the odb file, with no in-memory DFT state.
puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  -threads [thread_count] \
  [file join [file dirname [info script]] "scan_opt_rebalance_sky130_reload.tcl"] 2>@1]
