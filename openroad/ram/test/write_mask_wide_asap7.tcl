# A word of more than 64 bits split by -mask_size, no -bit_folds: each
# mask slice has its own write enable and takes, so the buffers between
# them and the slice's bits are the slice's own. Two write ports write
# slices of the same word with enables drawn independently, the last
# port winning a slice both write, and every slice reads back what was
# written to it; no slice's enable is left without a load. A slice of
# eight bits is one group, its enable and takes unbuffered, however wide
# the word.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 8 -word_size 128 -num_words 4 -r_ports 2 \
  -w_ports 2 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux

dict for {net sinks} [ram_sim::word_fanouts] {
  ram_sim::check { $sinks > 0 } "$net, driven in a word, has no sinks"
}
set names {}
foreach inst [[ord::get_db_block] getInsts] {
  lappend names [$inst getName]
}
ram_sim::check { [lsearch -glob $names *we_buf*] < 0 } \
  "an 8-bit slice has a write-enable group buffer"
ram_sim::check { [lsearch -glob $names *take_buf*] < 0 } \
  "an 8-bit slice has a take group buffer"

set we0 {}
set we1 {}
for { set i 0 } { $i < 16 } { incr i } {
  lappend we0 "we0\[$i\]"
  lappend we1 "we1\[$i\]"
}
ram_sim::load
ram_sim::exercise [list \
  words 4 bits 128 slices independent cycles 100 \
  writes [list [list addr_w0 D0 $we0] [list addr_w1 D1 $we1]] \
  reads {{addr_r0 Q0} {addr_r1 Q1}}]

puts "pass"
