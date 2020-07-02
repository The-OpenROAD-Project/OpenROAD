read_lef "merged.lef"
read_liberty "merged.lib"
read_def "no_sinks.def"
read_verilog "no_sinks.v"
read_sdc "no_sinks.sdc"

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf CLKBUF_X3 \
                     -wire_unit 20 \
                     -clk_nets "clk" 

exit
