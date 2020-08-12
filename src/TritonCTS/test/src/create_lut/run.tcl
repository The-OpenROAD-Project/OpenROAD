read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

current_design test_16_sinks

create_clock -name clk -period 5.0000 -waveform {0.0000 2.5000} [get_ports {clk}]

clock_tree_synthesis -buf_list "BUF_X1" \
                     -characterization_only \
                     -max_slew 10.0e-12 \
                     -max_cap 30.0e-15 \
                     -sqr_cap 7.7161e-5 \
                     -sqr_res 3.8e-1

exit
