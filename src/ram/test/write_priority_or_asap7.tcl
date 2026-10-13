# -write_priority or: where both write ports write a word in one cycle it
# takes the OR of their data, as firtool's register files describe it.
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
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 2 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux -write_priority or

ram_sim::load
ram_sim::exercise {
  words 8 bits 8 priority or cycles 300
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
