# -register_read_address 1: a read's data follows a clock after its address, and
# does not follow an address change before that clock.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -register_read_address 1
routed::check

ram_sim::load
set mem [ram_sim::exercise {
  words 8 bits 8 latency 1 cycles 100
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}]
# the address registered: a new address without a clock changes nothing
ram_sim::set_bus addr_r0 3 0
ram_sim::cycle clk
set before [ram_sim::get_bus Q0 8]
ram_sim::set_bus addr_r0 3 5
ram_sim::settle
ram_sim::check { [ram_sim::get_bus Q0 8] == $before } \
  "Q0 followed its address before the clock"
ram_sim::cycle clk
ram_sim::check { [ram_sim::get_bus Q0 8] == [lindex $mem 5] } \
  "Q0 does not read word 5 a clock after its address"

puts "pass"
