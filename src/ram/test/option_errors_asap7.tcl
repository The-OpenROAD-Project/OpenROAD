# Out-of-range options stop generation, each with its own message.

source "helpers.tcl"
source "ram_sim.tcl"

read_liberty ../../../test/asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty ../../../test/asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty ../../../test/asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz

read_lef ../../../test/asap7/asap7_tech_1x_201209.lef
read_lef ../../../test/asap7/asap7sc7p5t_28_R_1x_220121a.lef


set base {-mask_size 8 -word_size 8 -num_words 8 -r_ports 1 -w_ports 1
  -storage_cell DFFHQNx1_ASAP7_75t_R}
foreach {id options} {
  36 {-r_ports 0 -rb_ports 0}
  96 {-zero_word -2}
  92 {-banks 0}
  92 {-bank_columns -1}
  95 {-service_columns {0 10}}
  93 {-bit_folds 0}
  71 {-register_read_address -1}
  94 {-banks 2 -rb_ports -1}
} {
  set args [dict merge $base $options]
  if { ![catch { generate_ram_netlist {*}$args } message] } {
    error "generate_ram_netlist $options did not stop"
  }
  if { ![string match "*RAM-00$id*" $message] } {
    error "generate_ram_netlist $options stopped with: $message, not RAM-00$id"
  }
  puts "$options: RAM-00$id"
}

puts "pass"
