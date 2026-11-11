# A routed block too short for its address pins on the right edge: the
# ports the edge does not hold take theirs on the bottom edge. 8 words of
# 8 bits with two read and four write ports have 18 address pins for a
# 2.43 um edge, and pin placement stopped (PPL-0111).
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 4 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux -write_priority or \
  -decoder word
routed::check

# some address pins on the bottom edge, none on the left
set block [ord::get_db_block]
set die [$block getDieArea]
set bottom 0
foreach bterm [$block getBTerms] {
  if { ![string match addr_* [$bterm getName]] } {
    continue
  }
  set box [lindex [[lindex [$bterm getBPins] 0] getBoxes] 0]
  if { [$box yMin] == [$die yMin] } {
    incr bottom
  }
  ram_sim::check { [$box xMin] > [$die xMin] } \
    "[$bterm getName] is on the left edge"
}
ram_sim::check { $bottom > 0 } "no address pin on the bottom edge"

ram_sim::load
ram_sim::exercise {
  words 8 bits 8 priority or cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}
          {addr_w2 D2 {we2[0]}} {addr_w3 D3 {we3[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
