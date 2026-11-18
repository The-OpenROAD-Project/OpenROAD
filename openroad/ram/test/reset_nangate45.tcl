# -reset_value on Nangate45 with no -storage_cell: generate_ram picks a
# flop with an asynchronous clear and preset, and a plain flop for the
# registered read address, though every flop Nangate45 has drives both
# Q and QN. Every word reads 0x5a after reset.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_nangate45

generate_ram {*}$routed::nangate45 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 1 -w_ports 1 \
  -reset_value 0x5a -register_read_address 1
routed::check

ram_sim::load
foreach f $ram_sim::flops {
  set ram_sim::state([lindex $f 0]) [expr { int(rand() * 2) }]
}
ram_sim::set_port rst_n 0
for { set a 0 } { $a < 8 } { incr a } {
  ram_sim::set_bus addr_r0 3 $a
  ram_sim::cycle clk
  set q [ram_sim::get_bus Q 8]
  ram_sim::check { $q == 0x5a } "word $a reads $q after reset, not 0x5a"
}
ram_sim::set_port rst_n 1
ram_sim::exercise {
  words 8 bits 8 latency 1 mem {90 90 90 90 90 90 90 90}
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q}}
}

puts "pass"
