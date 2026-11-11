# The read side's drive: with -decoder word, a port that decodes more than
# eight words makes its address literals a buffer and an inverter per
# group of eight words, and an AND-OR read's word select reaches the
# word's bits through a buffer per group of eight bits. No net the
# address literals or the read selects drive has more than a group's
# sinks (an address inverter drove half the words' decoders, a read
# select a whole word's read gates). The array still reads back what was
# written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 32 -word_size 32 -num_words 32 -r_ports 2 \
  -w_ports 1 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux \
  -decoder word

# the most sinks a net driven by a decoder's literal or a select buffer has
set worst 0
set worst_net ""
foreach net [[ord::get_db_block] getNets] {
  set driver ""
  set sinks 0
  foreach iterm [$net getITerms] {
    if { [$iterm isOutputSignal] } {
      set driver [[$iterm getInst] getName]
    } elseif { [$iterm isInputSignal] } {
      incr sinks
    }
  }
  set literal [regexp {^decoder_p[0-9]+\.(inv|buf)_p} $driver]
  set select [regexp {^sel_buf_r_p} $driver]
  if { ($literal || $select) && $sinks > $worst } {
    set worst $sinks
    set worst_net [$net getName]
  }
}
ram_sim::check { $worst > 0 && $worst <= 8 } \
  "$worst_net, an address literal or read select, has $worst sinks; a group's is 8"

# a net named as a cell is refused by a Verilog reader
set cells [dict create]
foreach inst [[ord::get_db_block] getInsts] {
  dict set cells [$inst getName] 1
}
foreach net [[ord::get_db_block] getNets] {
  ram_sim::check { ![dict exists $cells [$net getName]] } \
    "net [$net getName] has a cell's name"
}

ram_sim::load
ram_sim::exercise {
  words 32 bits 32 cycles 300
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
