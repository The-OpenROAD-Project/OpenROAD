# -decoder word: each word's select is a balanced tree of AND2s over its
# whole address, ceil(log2(5)) = 3 deep for 32 words, and one more where
# the select meets the write enable: 4, where the row decoder's chain of
# AND2s makes 5.

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
  -mask_size 4 -word_size 4 -num_words 32 -r_ports 1 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -decoder word

ram_sim::load
set d [ram_sim::depth AND2*]
ram_sim::check { $d <= 4 } "the decode is $d AND2s deep, not 4"
ram_sim::exercise {
  words 32 bits 4
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q}}
}

puts "pass"
