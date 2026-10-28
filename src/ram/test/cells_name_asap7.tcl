# -cells and -name: the cell given for a role is the one used, and the
# block takes the name given.

source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -name regfile \
  -cells {inv INVx2_ASAP7_75t_R buffer BUFx4_ASAP7_75t_R}
routed::check

ram_sim::check { [[ord::get_db_block] getName] eq "regfile" } \
  "the block is [[ord::get_db_block] getName]"
foreach inst [[ord::get_db_block] getInsts] {
  set master [[$inst getMaster] getName]
  if { [string match INV* $master] } {
    ram_sim::check { $master eq "INVx2_ASAP7_75t_R" } "[$inst getName] is $master"
  }
  if { [string match BUF* $master] } {
    ram_sim::check { $master eq "BUFx4_ASAP7_75t_R" } "[$inst getName] is $master"
  }
}
ram_sim::check { [llength [ram_sim::insts_of INVx2_ASAP7_75t_R]] > 0 } "no INVx2"
ram_sim::check { [llength [ram_sim::insts_of BUFx4_ASAP7_75t_R]] > 0 } "no BUFx4"
ram_sim::load
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
