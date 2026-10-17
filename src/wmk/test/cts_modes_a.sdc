create_clock -name clock_mode_a -period 2 [get_pins leaf_a/Z]
set_propagated_clock [all_clocks]
