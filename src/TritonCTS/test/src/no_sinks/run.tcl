read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "no_sinks.def"

create_clock -period 5 clk

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf CLKBUF_X3 \
                     -wire_unit 20 \
                     -clk_nets "clk" 

exit
