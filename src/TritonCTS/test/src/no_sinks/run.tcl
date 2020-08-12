read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "no_sinks.def"

current_design test_no_sinks

create_clock -name clk -period 5.0000 -waveform {0.0000 2.5000} [get_ports {clk}]

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf CLKBUF_X3 \
                     -wire_unit 20 \
                     -clk_nets "clk" 

exit
