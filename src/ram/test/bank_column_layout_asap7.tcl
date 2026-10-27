# -bank_column_layout contiguous: with two words a row, each word's bits
# sit side by side, the other word's to one side, not interleaved, and
# a word written in two mask slices keeps both slices together.

source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 4 -word_size 8 -num_words 32 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -banks 4 -bank_columns 2 \
  -bank_column_layout contiguous
routed::check

ram_sim::load
set ranges [dict create]
dict for {word insts} [ram_sim::words_of_flops] {
  set xs [lmap i $insts { lindex [$i getLocation] 0 }]
  set range [list [tcl::mathfunc::min {*}$xs] [tcl::mathfunc::max {*}$xs]]
  dict lappend ranges [lindex $word 0] $range
}
dict for {row rs} $ranges {
  lassign [lsort -integer -index 0 $rs] a b
  ram_sim::check { [lindex $a 1] < [lindex $b 0] } \
    "row $row: the words' bits are interleaved ($a, $b)"
}
ram_sim::exercise {
  words 32 bits 8
  writes {{addr_w D {{we[0]} {we[1]}}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
