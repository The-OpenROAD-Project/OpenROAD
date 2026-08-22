# Compare full timing-update runtime for the Elmore and Lambert-W delay calculators.
set test_name dcalc_full_update_runtime
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_lef example1.lef
read_liberty example1_slow.lib
read_def example1.def
read_spef example1.dspef

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk 0 out

run_dcalc_full_update_runtime
