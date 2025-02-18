source "helpers.tcl"
source "cts-helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set block [make_array 301]

set inst [$block findInst "ff300"]
$inst setOrigin 99000 795000

sta::db_network_defined

create_clock -period 5 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

clock_tree_synthesis -root_buf CLKBUF_X3 \
                     -buf_list CLKBUF_X3 \
                     -wire_unit 20 \
                     -sink_clustering_enable \
                     -distance_between_buffers 100 \
                     -num_static_layers 1
