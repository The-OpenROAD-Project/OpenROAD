# -periphery verilog: the block is the core alone, <name>_core, and the
# periphery a Verilog module, <name>, around it. Simulated together, the
# wrapper and the core read back every word written.

source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

set wrapper [make_result_file periphery_verilog_asap7_wrapper.v]
generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -name rf \
  -periphery verilog -wrapper_verilog $wrapper
routed::check

ram_sim::check { [[ord::get_db_block] getName] eq "rf_core" } \
  "the block is [[ord::get_db_block] getName], not rf_core"
foreach inst [[ord::get_db_block] getInsts] {
  ram_sim::check { ![string match decoder* [$inst getName]] } \
    "the core holds the periphery's [$inst getName]"
}
ram_sim::load
ram_sim::add_wrapper $wrapper
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
