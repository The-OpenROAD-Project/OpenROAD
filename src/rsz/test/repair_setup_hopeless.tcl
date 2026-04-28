# repair_timing on a hopelessly under-clocked design must terminate
# without grinding through the entire endpoint list one futile pass at a
# time. The block below has many independent register-to-register pairs,
# a clock period below clk-to-Q + setup, and dont_use + skip_* tuned so
# no move can improve WNS. Without the WNS-stagnation gate, repair_timing
# walks the whole 1200-endpoint list; with the gate it exits shortly
# after the warmup. The .ok pins the bounded behaviour.

source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits 2000
set m1 [$tech findLayer metal1]
set dff [$db findMaster DFF_X2]
set dff_d [$dff findMTerm D]
set dff_q [$dff findMTerm Q]
set dff_ck [$dff findMTerm CK]

set clk_net [odb::dbNet_create $block clk]
set clk_bt [odb::dbBTerm_create $clk_net clk]
$clk_bt setIoType INPUT
set bp [odb::dbBPin_create $clk_bt]
odb::dbBox_create $bp $m1 0 0 100 100
$bp setPlacementStatus LOCKED

set num_pairs 1200
for { set i 0 } { $i < $num_pairs } { incr i } {
  set y [expr { $i * 2000 }]
  set src [odb::dbInst_create $block $dff "src$i"]
  $src setLocation 0 $y
  $src setPlacementStatus LOCKED
  [$src getITerm $dff_ck] connect $clk_net
  set dst [odb::dbInst_create $block $dff "dst$i"]
  $dst setLocation 2000 $y
  $dst setPlacementStatus LOCKED
  [$dst getITerm $dff_ck] connect $clk_net
  set n [odb::dbNet_create $block "n$i"]
  [$src getITerm $dff_q] connect $n
  [$dst getITerm $dff_d] connect $n
}
ord::design_created

create_clock -period 0.001 clk
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# DFF_X2 is the largest DFF in Nangate45; with everything but _X1/_X2
# off-limits, SizeUpMove has nothing to size up.
foreach lib_cell [get_lib_cells *] {
  if { ![regexp {_X[12]$} [get_name $lib_cell]] } {
    set_dont_use $lib_cell
  }
}

repair_timing -setup -skip_buffer_removal -skip_pin_swap -skip_gate_cloning \
  -skip_buffering
report_worst_slack -max
