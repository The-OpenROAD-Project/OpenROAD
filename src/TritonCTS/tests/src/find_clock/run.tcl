read_lef "merged.lef"
read_liberty "merged.lib"
read_def "aes.def"
read_verilog "aes.v"
read_sdc "aes.sdc"

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf CLKBUF_X3 \
                     -wire_unit 20 

