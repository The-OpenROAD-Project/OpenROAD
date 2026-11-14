# -w_port_words with -bit_folds 3: a 7-bit word in three rows of 3, 3 and
# 1 bits, its write logic built in the first row and shared by the other
# two. Three write ports with overlapping lists, the last one writing a
# word in a cycle winning.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

set reach {{0 1 2 3} {2 3 4 5} {5 6 7 0}}
generate_ram_netlist -mask_size 3 -word_size 7 -num_words 8 -r_ports 2 \
  -w_ports 3 -storage_cell DFFHQNx1_ASAP7_75t_R -decoder word -bit_folds 3 \
  -w_port_words $reach

ram_sim::load
ram_sim::exercise {
  words 8 bits 7 priority last cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}} {addr_w2 D2 {we2[0]}}}
  write_words {{0 1 2 3} {2 3 4 5} {5 6 7 0}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
