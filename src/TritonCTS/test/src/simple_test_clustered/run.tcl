read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "aes.def"

create_clock -period 5 clk

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf CLKBUF_X3 \
                     -wire_unit 20 \
                     -post_cts_disable \
                     -sink_clustering_enable \
                     -distance_between_buffers 100 \
                     -sink_clustering_size 10 \
                     -sink_clustering_max_diameter 60 \
                     -num_static_layers 1

exit
