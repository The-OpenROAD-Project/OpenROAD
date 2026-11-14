# -w_port_words with lists that overlap: word 1 is written by ports 0, 1
# and 2, word 6 by ports 0 and 2, every other word by one port. Where
# ports write the same word in a cycle the last one wins
# (-write_priority last), through the clock gate (-write_style
# clock_gate). The behavioral model drops a write outside a port's list,
# as the netlist does.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

set reach {{0 1 4 6} {1 5} {2 6 1} {3 7}}
set model [make_result_file write_port_words_overlap_asap7.v]
generate_ram_netlist -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 \
  -w_ports 4 -storage_cell DFFHQNx1_ASAP7_75t_R -decoder word \
  -write_style clock_gate -write_priority last -w_port_words $reach \
  -write_behavioral_verilog $model

set names {}
foreach inst [[ord::get_db_block] getInsts] {
  lappend names [$inst getName]
}
ram_sim::check { [lsearch -glob $names *decoder_p1*word0] < 0 } \
  "port 1 decodes no word it does not write"
ram_sim::check { [lsearch -glob $names *decoder_p2*word1] >= 0 } \
  "port 2 decodes word 1"

# each port's write in the model is conditioned on its words
set f [open $model]
set text [read $f]
close $f
foreach port {0 1 2 3} {
  set words [lsort -integer -unique [lindex $reach $port]]
  set cond [join [lmap w $words { string cat "addr_w$port == $w" }] " || "]
  ram_sim::check { [string first "we$port\[i\] && ($cond)" $text] >= 0 } \
    "the model's port $port writes words other than $words"
}

ram_sim::load
ram_sim::exercise {
  words 8 bits 8 priority last cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}
          {addr_w2 D2 {we2[0]}} {addr_w3 D3 {we3[0]}}}
  write_words {{0 1 4 6} {1 5} {2 6 1} {3 7}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
