# The shape of an ORFS asap7 design's constraints after write_sdc has had
# its way with them: [all_inputs] and [all_registers] spelled out object
# by object, in three set_max_delay and four group_path commands.
create_clock -name clk -period 10 [get_ports clk1]
create_clock -name clk2 -period 20 [get_ports clk2]
set_clock_uncertainty 0.1 clk
set_clock_latency 0.5 [get_clocks clk]
set_propagated_clock [get_clocks clk2]
set_clock_groups -name grp -asynchronous -group clk -group clk2
set_false_path -from [get_ports in1]
set non_clk_inputs [all_inputs -no_clocks]
set_max_delay -ignore_clock_latency 8 -from $non_clk_inputs -to [all_registers]
set_max_delay -ignore_clock_latency 8 -from [all_registers] -to [all_outputs]
set_max_delay 8 -from $non_clk_inputs -to [all_outputs]
group_path -name in2reg -from $non_clk_inputs -to [all_registers]
group_path -name reg2out -from [all_registers] -to [all_outputs]
group_path -name reg2reg -from [all_registers] -to [all_registers]
group_path -name in2out -from $non_clk_inputs -to [all_outputs]
set_multicycle_path -setup 2 -from [get_pins r1/CP] -to [get_pins r3/D]
set_input_delay 1 -clock clk [get_ports in2]
set_output_delay 2 -clock clk2 [get_ports out]
set_max_fanout 32 [current_design]
set_max_transition 0.5 [current_design]
set_case_analysis 0 [get_ports in1]
