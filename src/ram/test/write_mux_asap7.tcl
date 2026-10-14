# -write_style mux: no clock gate; every storage flop is on the clock, and
# a word not written keeps its value through a mux per bit.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux
routed::check

ram_sim::load
ram_sim::check { [llength $ram_sim::gates] == 0 } "a clock gate is left"
set clk [ram_sim::net_of_port clk]
foreach f $ram_sim::flops {
  set pins [lindex $f 1]
  ram_sim::check { [dict get $pins CLK] == $clk } \
    "[[lindex $f 0] getName] is not on the clock"
}
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
