current_design clk_passthrough_top
set clk_period 500
create_clock -name clk -period $clk_period [get_ports clk]
set_input_delay  [expr $clk_period * 0.2] -clock clk [all_inputs -no_clocks]
set_output_delay [expr $clk_period * 0.2] -clock clk [all_outputs]
set_false_path -from [get_ports reset]