create_clock -name clock_mode_b -period 3 [get_pins leaf_b/Z]
set_propagated_clock [all_clocks]
