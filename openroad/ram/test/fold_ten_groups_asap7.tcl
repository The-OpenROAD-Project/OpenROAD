# A 60-bit word in five folds of 12 bits: two groups a fold, ten group
# buffers in all, more than eight, so the word's enable and each port's
# take reach them through a buffer per eight, and none drives more than
# eight. Counting the word's bits, not the groups the folds build, saw
# eight groups and left the enable driving ten. The array still reads
# back what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 12 -word_size 60 -num_words 4 -r_ports 2 \
  -w_ports 2 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux \
  -decoder word -bit_folds 5

lassign [ram_sim::worst_fanout] worst_net worst
ram_sim::check { $worst <= 9 } \
  "$worst_net, driven in a word, has $worst sinks; a group's is 9"

ram_sim::load
ram_sim::exercise {
  words 4 bits 60 cycles 100
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
