# -bank_column_layout contiguous: with two words a row, each word's bits
# sit side by side, the other word's to one side, not interleaved.

source "helpers.tcl"
source "ram_sim.tcl"

read_liberty ../../../test/asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty ../../../test/asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz

read_lef ../../../test/asap7/asap7_tech_1x_201209.lef
read_lef ../../../test/asap7/asap7sc7p5t_28_R_1x_220121a.lef


generate_ram_netlist \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -banks 4 -bank_columns 2 \
  -bank_column_layout contiguous

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
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
