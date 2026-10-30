# generate_ram on asap7, which has no tristate cell: the read ports
# are AND-OR trees, not tristate bitlines, and every word reads back what
# was written, through a flop whose only output is its data inverted.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R
routed::check

ram_sim::load
ram_sim::check { [ram_sim::tristate_count] == 0 } "a read path has a tristate"
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
