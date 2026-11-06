# -rb_ports 1 with 4 interleaved banks: a read port with an address and
# data per bank (addr_rb0_b<b>, Qrb0_b<b>); bank b's address i reads word
# i * 4 + b.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 16 -r_ports 0 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R \
  -banks 4 -bank_order interleaved -rb_ports 1
routed::check

# bank b's address i holds word i * 4 + b
proc interleaved { bank i } {
  return [expr { $i * 4 + $bank }]
}

ram_sim::load
ram_sim::exercise {
  words 16 bits 8
  writes {{addr_w D {we[0]}}}
  banked_reads {{addr_rb0_b0 Qrb0_b0 0} {addr_rb0_b1 Qrb0_b1 1}
                {addr_rb0_b2 Qrb0_b2 2} {addr_rb0_b3 Qrb0_b3 3}}
  bank_word interleaved
}

puts "pass"
