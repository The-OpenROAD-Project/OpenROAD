# -service_columns {4 10}: an empty column 10 sites wide after every 4
# storage columns (two in an 8-bit word with its select column), every
# row clear there.

source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram {*}$routed::asap7 \
  -mask_size 8 -word_size 8 -num_words 8 -r_ports 2 -w_ports 1 \
  -storage_cell DFFHQNx1_ASAP7_75t_R -service_columns {4 10}
routed::check

set block [ord::get_db_block]
set site [[lindex [[lindex [[ord::get_db] getLibs] 0] getSites] 0] getWidth]
set spans {}
# fillers and taps fill the routed block's empty columns
foreach inst [$block getInsts] {
  set type [[$inst getMaster] getType]
  if { [$inst isPlaced] && $type != "CORE_SPACER" && $type != "CORE_WELLTAP" } {
    set b [$inst getBBox]
    lappend spans [list [$b xMin] [$b xMax]]
  }
}
set gaps {}
set edge [[$block getDieArea] xMin]
foreach s [lsort -integer -index 0 $spans] {
  lassign $s a b
  if { $a - $edge >= 10 * $site } {
    lappend gaps [expr { ($a - $edge) / $site }]
  }
  set edge [expr { max($edge, $b) }]
}
ram_sim::check { [llength $gaps] == 2 && [lsort -unique $gaps] eq {10} } \
  "empty columns of $gaps sites, not two of 10"
ram_sim::load
ram_sim::exercise {
  words 8 bits 8
  writes {{addr_w D {we[0]}}}
  reads {{addr_r0 Q0} {addr_r1 Q1}}
}

puts "pass"
