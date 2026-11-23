# Mask slices of more than eight groups, no -bit_folds: a 144-bit word
# in two slices of 72 bits, nine groups each. Each slice's enable and
# takes reach its group buffers through buffers of its own, not through
# those another slice's enable drives, so no slice's enable is left
# without a load and no net drives more than a group's pins. Two write
# ports write slices of the same word with enables drawn independently,
# and every slice reads back what was written to it.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 72 -word_size 144 -num_words 4 -r_ports 2 \
  -w_ports 2 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux

dict for {net sinks} [ram_sim::word_fanouts] {
  ram_sim::check { $sinks > 0 } "$net, driven in a word, has no sinks"
}
lassign [ram_sim::worst_fanout] worst_net worst
ram_sim::check { $worst <= 9 } \
  "$worst_net, driven in a word, has $worst sinks; a group's is 9"

ram_sim::load
ram_sim::exercise {
  words 4 bits 144 slices independent cycles 100
  writes {{addr_w0 D0 {we0[0] we0[1]}} {addr_w1 D1 {we1[0] we1[1]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
