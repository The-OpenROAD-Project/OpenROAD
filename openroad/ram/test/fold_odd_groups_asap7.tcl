# A 72-bit word in eight folds of 9 bits: each fold builds a group of 8
# and a group of 1, sixteen group buffers in all, so the word's enable
# and each port's take reach them through two buffers of eight, and none
# drives more than eight. Numbering the groups by bit, not as the folds
# build them, put fifteen behind the first. The array still reads back
# what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 9 -word_size 72 -num_words 4 -r_ports 2 \
  -w_ports 2 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux \
  -decoder word -bit_folds 8

lassign [ram_sim::worst_fanout] worst_net worst
ram_sim::check { $worst <= 9 } \
  "$worst_net, driven in a word, has $worst sinks; a group's is 9"

ram_sim::load
ram_sim::exercise {
  words 4 bits 72 cycles 100
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
