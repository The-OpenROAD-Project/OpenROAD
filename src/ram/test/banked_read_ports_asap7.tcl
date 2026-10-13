# -rb_ports 1 with 4 interleaved banks: a read port with an address and
# data per bank (addr_rb0_b<b>, Qrb0_b<b>); bank b's address i reads word
# i * 4 + b.
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
  -mask_size 8 -word_size 8 -num_words 16 -r_ports 0 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R \
  -banks 4 -bank_order interleaved -rb_ports 1

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
