# -banks 4 -bank_columns 2 -bank_order interleaved: the words in four
# banks, word w in bank w % 4, two banks side by side in a row; every word
# is decoded whole and reads back what was written.
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
  -storage_cell DFFHQNx1_ASAP7_75t_R \
  -banks 4 -bank_columns 2 -bank_order interleaved

ram_sim::load
# two words a row: 8 words in 4 rows of storage
set rows {}
foreach f $ram_sim::flops {
  lappend rows [lindex [[lindex $f 0] getLocation] 1]
}
ram_sim::check { [llength [lsort -unique $rows]] == 4 } \
  "storage in [llength [lsort -unique $rows]] rows, not 4"
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
