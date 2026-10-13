# -bit_folds 3 on a 7-bit word: each word in three rows of 3, 3 and 1
# bits, written whole and read back.

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
  -mask_size 3 -word_size 7 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -decoder word -bit_folds 3

ram_sim::load
ram_sim::check { [llength $ram_sim::flops] == 8 * 7 } \
  "[llength $ram_sim::flops] storage flops, not 8 words of 7"
dict for {word insts} [ram_sim::words_of_flops] {
  set ys [lsort -unique [lmap i $insts { lindex [$i getLocation] 1 }]]
  ram_sim::check { [llength $ys] == 3 } "word $word is in [llength $ys] rows, not 3"
}
ram_sim::exercise {
  words 8 bits 7
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
