source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_liberty Nangate45/Nangate45_fast.lib

read_verilog scan_architect_no_mix_nangate45.v
link_design scan_architect_no_mix_nangate45

create_clock -name clk -period 2.0000 -waveform {0.0000 1.0000} [get_ports clk]

set_dft_config -max_length 10 -clock_mixing no_mix

scan_replace
report_dft_plan -verbose
execute_dft_plan

set verilog_file [make_result_file scan_architect_no_mix_nangate45.v]
write_verilog -sort $verilog_file
diff_files $verilog_file scan_architect_no_mix_nangate45.vok
