source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers3.def

set constrained_buffer [get_cells b2]
puts "Constrained buffer instances: [llength $constrained_buffer]"
set_false_path -through $constrained_buffer

remove_buffers b2
puts "Buffer remains: [llength [get_cells -quiet b2]]"
