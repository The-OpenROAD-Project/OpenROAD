# -zero_word 0: word 0 reads 0 whatever is written to it, as a RISC-V
# register file's x0, and holds no flops; every other word reads back.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -zero_word 0
routed::check

ram_sim::load
ram_sim::check { [llength $ram_sim::flops] == 7 * 8 } \
  "[llength $ram_sim::flops] storage flops, not 7 words of 8"
ram_sim::exercise {
  words 8 bits 8 zero_word 0
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
