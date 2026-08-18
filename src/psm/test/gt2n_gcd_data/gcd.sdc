set clk_period 690
set clk_io_pct 0.2

set clk_port [get_ports clk]

create_clock -name clk -period $clk_period $clk_port

set non_clock_inputs [lsearch -inline -all -not -exact [all_inputs] $clk_port]
set_input_delay [expr {$clk_period * $clk_io_pct}] -clock clk $non_clock_inputs
set_output_delay [expr {$clk_period * $clk_io_pct}] -clock clk [all_outputs]

set_driving_cell -lib_cell gt2_6t_buf_x1_w31_svt [all_inputs]
