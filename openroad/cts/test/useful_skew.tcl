# Verify clock_tree_synthesis -useful_skew improves timing by realizing the
# resizer's per-register skew targets with delay buffers.
#
# The design is built programmatically: several register pipelines
# r1 -> (long buffer chain) -> r2 -> r3 where the r1->r2 stage is
# setup-critical and the r2->r3 stage is loose. Delaying r2's clock
# borrows time from the loose stage to help the critical one, so useful
# skew produces a non-zero schedule that the second latency-balancing
# pass realizes with delay buffers.
#
# An array_tile macro (with insertion delay) is added so CTS builds a
# MacroTree/RegisterTree hierarchy; the useful-skew realization pass runs
# only on such a hierarchical clock tree.
#
# All timing paths are register-to-register, so the clock's common
# insertion delay cancels and only the useful skew changes reg-to-reg
# slack. The test measures WNS/TNS before CTS (ideal clock) and after CTS
# -useful_skew (propagated clock) and asserts both improve. The benefit is
# reported as a boolean, not exact slack values, so the golden stays
# stable across STA changes.
source "helpers.tcl"
source Nangate45/Nangate45.vars

read_liberty $liberty_file
read_liberty array_tile_ins_delay.lib
read_lef Nangate45/Nangate45.lef
read_lef array_tile_ins_delay.lef

set num_chains 8
set chain_len 16
set clk_period 0.30
set row_pitch 6

set db [ord::get_db]
set tech [ord::get_db_tech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip "useful_skew_top"]
$block setDefUnits 2000
set dbu [$tech getDbUnitsPerMicron]

set dff [$db findMaster "DFF_X1"]
set buf [$db findMaster "BUF_X1"]

# Clock input port/net.
set clk [odb::dbNet_create $block "clk"]
$clk setSigType CLOCK
set clk_bterm [odb::dbBTerm_create $clk "clk"]
$clk_bterm setSigType CLOCK
$clk_bterm setIoType INPUT
set clk_bpin [odb::dbBPin_create $clk_bterm]
$clk_bpin setPlacementStatus PLACED
odb::dbBox_create $clk_bpin [$tech findLayer "metal5"] 0 0 2000 2000

proc place_inst { inst x_um y_um dbu } {
  $inst setLocation [expr int($x_um * $dbu)] [expr int($y_um * $dbu)]
  $inst setPlacementStatus PLACED
}

# Register pipelines with an asymmetric (tight r1->r2, loose r2->r3) slack.
for { set c 0 } { $c < $num_chains } { incr c } {
  set yy [expr 2 + $c * $row_pitch]
  set r1 [odb::dbInst_create $block $dff "r1_$c"]
  set r2 [odb::dbInst_create $block $dff "r2_$c"]
  set r3 [odb::dbInst_create $block $dff "r3_$c"]
  place_inst $r1 10 $yy $dbu
  place_inst $r2 185 $yy $dbu
  place_inst $r3 192 $yy $dbu
  foreach r [list $r1 $r2 $r3] {
    [$r findITerm "CK"] connect $clk
  }

  # Tight stage: r1.Q -> chain of buffers -> r2.D
  set prev [$r1 findITerm "Q"]
  for { set i 0 } { $i < $chain_len } { incr i } {
    set b [odb::dbInst_create $block $buf "u_${c}_$i"]
    place_inst $b [expr 20 + $i * 10] $yy $dbu
    set net [odb::dbNet_create $block "s1_${c}_$i"]
    $prev connect $net
    [$b findITerm "A"] connect $net
    set prev [$b findITerm "Z"]
  }
  set net [odb::dbNet_create $block "s1l_$c"]
  $prev connect $net
  [$r2 findITerm "D"] connect $net

  # Loose stage: r2.Q -> r3.D
  set net [odb::dbNet_create $block "s2_$c"]
  [$r2 findITerm "Q"] connect $net
  [$r3 findITerm "D"] connect $net
}

# Macro sink (insertion delay) so CTS builds a hierarchical clock tree.
set tile [$db findMaster "array_tile"]
set macro [odb::dbInst_create $block $tile "macro0"]
place_inst $macro 40 [expr 6 + $num_chains * $row_pitch] $dbu
[$macro findITerm "clk"] connect $clk

ord::design_created

set die_h [expr 4 + $num_chains * $row_pitch + 80]
initialize_floorplan \
  -die_area [list 0 0 200 $die_h] \
  -core_area [list 0 0 200 $die_h] \
  -site $site
source $tracks_file

source Nangate45/Nangate45.rc
create_clock -name clk -period $clk_period clk
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
estimate_parasitics -placement

# Baseline timing with the ideal clock (before the useful-skew tree).
set wns_base [sta::worst_slack -max]
set tns_base [sta::total_negative_slack -max]

set_cts_config -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -sink_clustering_size 8 \
  -sink_clustering_max_diameter 60

clock_tree_synthesis -sink_clustering_enable -useful_skew

# Timing with the realized useful-skew clock tree.
set_propagated_clock [all_clocks]
estimate_parasitics -placement
set wns_skew [sta::worst_slack -max]
set tns_skew [sta::total_negative_slack -max]

# WNS/TNS are negative; useful skew improves timing when the slack is less
# negative, i.e. the value is greater.
puts "Useful skew improves WNS: [expr { $wns_skew > $wns_base }]"
puts "Useful skew improves TNS: [expr { $tns_skew > $tns_base }]"
