# -write_priority or: where both write ports write a word in one cycle it
# takes the OR of their data.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 2 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux -write_priority or
routed::check

ram_sim::load
ram_sim::exercise {
  words 8 bits 8 priority or cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
