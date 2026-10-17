# One construct the native form does not carry (a derate), so the whole
# Sdc rides as write_sdc text and is replayed. Nothing is dropped.
create_clock -name clk -period 10 [get_ports clk1]
set_input_delay 1 -clock clk [get_ports in1]
set_timing_derate -early 0.95
set_timing_derate -late 1.05
