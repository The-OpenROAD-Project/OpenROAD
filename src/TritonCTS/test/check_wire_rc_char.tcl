read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

#Modified from create_lut.tcl
#
create_clock -period 5 clk

set_wire_rc -clock -layer metal5
configure_cts_characterization

clock_tree_synthesis -buf_list "BUF_X1" \
                     -characterization_only
