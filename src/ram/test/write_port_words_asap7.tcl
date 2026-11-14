# -w_port_words: each write port writes only the words it lists. A word
# written by one port takes that port's data straight, with no data mux
# and no OR of the ports' enables; a port builds no decoder for a word it
# does not write. The array still reads back what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

set base {-mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 4
  -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux -write_priority or}

# each refused, by its id, before any block is made
foreach {options id} {
  {-decoder word -w_port_words {{0 4} {1 5} {2 6}}} RAM-0128
  {-w_port_words {{0 4} {1 5} {2 6} {3 7}}} RAM-0129
  {-decoder word -w_port_words {{0 4} {1 5} {2 6} {3 9}}} RAM-0130
  {-decoder word -w_port_words {{0 4} {1 5} {2 6} {3}}} RAM-0131
  {-decoder word -rw_ports 1 -w_ports 0 -w_port_words {{0 1 2 3 4 5 6 7}}}
  RAM-0132
  {-decoder word -w_port_words {{0 x 4} {1 5} {2 6} {3 7}}} RAM-0133
  {-decoder word -w_port_words {{0 3 4 7} {1 5} {2 6} {}}} RAM-0134
  {-decoder word -w_port_words {{}}} RAM-0128
  {-decoder word -w_port_words {}} RAM-0135
} {
  set rc [catch { generate_ram_netlist {*}$base {*}$options } msg]
  ram_sim::check { $rc && [string match "*$id*" $msg] } \
    "$options stopped with '$msg', not $id"
}

# 8 words in two banks of four rows, as a bank fill writes them: port r
# writes row r of either bank, words r and r + 4
set reach {{0 4} {1 5} {2 6} {3 7}}
generate_ram_netlist {*}$base -decoder word -w_port_words $reach

set names {}
foreach inst [[ord::get_db_block] getInsts] {
  lappend names [$inst getName]
}
ram_sim::check { [lsearch -glob $names *dmux*] < 0 } \
  "a word written by one port has no data mux"
ram_sim::check { [lsearch -glob $names *any_or*] < 0 } \
  "a word written by one port has no OR of the ports' enables"
ram_sim::check { [lsearch -glob $names *decoder_p0*word1] < 0 } \
  "port 0 decodes no word it does not write"
ram_sim::check { [lsearch -glob $names *decoder_p0*word4] >= 0 } \
  "port 0 decodes word 4"

ram_sim::load
ram_sim::exercise {
  words 8 bits 8 priority or cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}
          {addr_w2 D2 {we2[0]}} {addr_w3 D3 {we3[0]}}}
  write_words {{0 4} {1 5} {2 6} {3 7}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
