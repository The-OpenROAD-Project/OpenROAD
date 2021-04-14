# find_fanin_fanouts
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg6.def

foreach pin [rsz::find_fanin_fanouts [get_pins r3/D]] {
  puts [get_full_name $pin]
}
