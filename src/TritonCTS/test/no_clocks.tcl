read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "no_clock.def"

set_wire_rc -clock -layer metal5
catch {clock_tree_synthesis \
         -root_buf CLKBUF_X3 \
         -buf_list CLKBUF_X3 \
         -wire_unit 20} error
puts $error
