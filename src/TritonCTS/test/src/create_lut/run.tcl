read_lef "merged.lef"
read_liberty "merged.lib"
read_def "aes.def"
read_verilog "aes.v"
read_sdc "aes.sdc"

clock_tree_synthesis -buf_list "BUF_X1" \
                     -characterization_only \
                     -max_slew 10.0e-12 \
                     -max_cap 30.0e-15 \
                     -sqr_cap 7.7161e-5 \
                     -sqr_res 3.8e-1

exit
