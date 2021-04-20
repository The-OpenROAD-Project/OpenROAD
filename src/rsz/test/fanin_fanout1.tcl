# find_fanin_fanouts
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def fanin_fanout1.def

report_object_full_names [rsz::find_fanin_fanouts [get_pins r3/D]]
report_object_full_names [rsz::find_fanin_fanouts [get_pins r1/D]]
