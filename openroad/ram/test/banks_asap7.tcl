# -banks 4 -bank_columns 2 -bank_order interleaved: 32 words in four
# banks, word w in bank w % 4, two banks side by side in a row; every word
# is decoded whole and reads back what was written.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 32 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R \
  -banks 4 -bank_columns 2 -bank_order interleaved
routed::check

ram_sim::load
# two words a row: 32 words in 16 rows of storage
set rows {}
foreach f $ram_sim::flops {
  lappend rows [lindex [[lindex $f 0] getLocation] 1]
}
ram_sim::check { [llength [lsort -unique $rows]] == 16 } \
  "storage in [llength [lsort -unique $rows]] rows, not 16"
ram_sim::exercise {
  words 32 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
