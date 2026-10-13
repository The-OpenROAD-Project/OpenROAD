# -service_columns {4 10}: an empty column 10 sites wide after every 4
# storage columns (two in an 8-bit word with its select column), every
# row clear there.

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
  -storage_cell DFFHQNx1_ASAP7_75t_R -service_columns {4 10}

set block [ord::get_db_block]
set site [[lindex [[lindex [[ord::get_db] getLibs] 0] getSites] 0] getWidth]
set spans {}
foreach inst [$block getInsts] {
  if { [$inst isPlaced] } {
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
