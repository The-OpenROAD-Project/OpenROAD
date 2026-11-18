# -write_style mux: a word's write enable and its hold reach the word's
# bits through a buffer and an inverter per group of eight bits, so no
# net in a word drives more than a group's pins. A 32-bit word's
# enable once drove 33 pins and its hold 32. The array still reads back
# what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 32 -word_size 32 -num_words 8 -r_ports 2 \
  -w_ports 1 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux

# the most sinks a net driven inside a word has
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
  if { [string match "storage_*" $driver] && $sinks > $worst } {
    set worst $sinks
    set worst_net [$net getName]
  }
}
ram_sim::check { $worst <= 9 } \
  "$worst_net, driven in a word, has $worst sinks; a group's is 9"

ram_sim::load
ram_sim::exercise {
  words 8 bits 32 cycles 300
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
