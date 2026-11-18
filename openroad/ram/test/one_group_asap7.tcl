# A word of no more than eight bits is one group: its write enable and
# each port's take reach its bits without a group buffer, which would
# only add a level. Wider words keep theirs. The array still reads back
# what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 \
  -w_ports 2 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux

set names {}
foreach inst [[ord::get_db_block] getInsts] {
  lappend names [$inst getName]
}
ram_sim::check { [lsearch -glob $names *we_buf*] < 0 } \
  "an 8-bit word has a write-enable group buffer"
ram_sim::check { [lsearch -glob $names *take_buf*] < 0 } \
  "an 8-bit word has a take group buffer"

ram_sim::load
ram_sim::exercise {
  words 8 bits 8 cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
