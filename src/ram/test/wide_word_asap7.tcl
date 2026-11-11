# A word of more than eight groups of eight bits: its write enable and
# each port's take reach the group buffers through a buffer per eight
# groups, shared by the word's folds, so neither drives more than eight
# group buffers. A 128-bit word's enable once drove 16. The array still
# reads back what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 16 -word_size 128 -num_words 4 -r_ports 2 \
  -w_ports 2 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux \
  -write_priority or -decoder word -bit_folds 8

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
  words 4 bits 128 priority or cycles 100
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
