# -reset_value 0xa5: an asynchronous reset at the storage flop's own
# polarity (rst_n for asap7's DFFASRHQNx1) sets every word to 0xa5, and
# the words are written and read as usual after it.
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
  -storage_cell DFFASRHQNx1_ASAP7_75t_R -reset_value 0xa5

ram_sim::load
# scramble the flops, then reset
foreach f $ram_sim::flops {
  set ram_sim::state([lindex $f 0]) [expr { int(rand() * 2) }]
}
ram_sim::set_port rst_n 0
ram_sim::cycle clk
for { set a 0 } { $a < 8 } { incr a } {
  ram_sim::set_bus addr_r0 3 $a
  ram_sim::settle
  set q [ram_sim::get_bus Q0 8]
  ram_sim::check { $q == 0xa5 } "word $a reads $q after reset, not 0xa5"
}
ram_sim::set_port rst_n 1
ram_sim::exercise {
  words 8 bits 8 mem {165 165 165 165 165 165 165 165}
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
