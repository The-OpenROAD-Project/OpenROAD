# fully_rebuffer with set_opt_config -rebuffer_size_driver checks the side
# loads of a driver's input net even when a resize earlier in the pass
# re-timed that net.  Inverter d drives net n, which feeds x, g and the
# failing output side.  g's output is two STA levels above x's, so g is
# upsized first and loads n more.  Re-timing n clears the required times of
# its loads, which lie below x's level.  The timing refresh at x's level
# computes them again, so the side-load check finds that INV_X4 on x would
# slow side below x's target slack, and keeps x at INV_X1 with a buffer.
# Skipping those loads as unconstrained would let x take INV_X4 and slow side
# further.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]

# Top-level port on net at (x, y) um.
proc make_port { net name io x y } {
  global block pin_layer
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
}

# Placed instance of master at (x, y) um with {term net} connections.
proc make_inst { master name conns x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  foreach { term net } $conns {
    [$inst findITerm $term] connect $net
  }
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
  $inst setPlacementStatus PLACED
}

# in -> d -> n -> {x, g/A1, side}; b -> i1 -> i2 -> g/A2 puts g two levels
# above x.  n is dont_touch, so only the output nets of x and g are rebuffered.
set in [odb::dbNet_create $block in]
set b [odb::dbNet_create $block b]
set n [odb::dbNet_create $block n]
set c1 [odb::dbNet_create $block c1]
set c2 [odb::dbNet_create $block c2]
set nx [odb::dbNet_create $block nx]
set ng [odb::dbNet_create $block ng]
make_port $in in INPUT 0 0
make_port $b b INPUT 0 10
make_port $n side OUTPUT 12 0
make_port $nx outx OUTPUT 20 0
make_port $ng outg OUTPUT 20 10
make_inst INV_X1 d [list A $in ZN $n] 2 0
make_inst INV_X1 x [list A $n ZN $nx] 8 0
make_inst INV_X4 i1 [list A $b ZN $c1] 2 10
make_inst INV_X4 i2 [list A $c1 ZN $c2] 4 10
make_inst NAND2_X1 g [list A1 $n A2 $c2 ZN $ng] 8 10
$n setDoNotTouch true
ord::design_created

# g's path from b is the worst.  side fails by less than x's path, but by
# more than x's path after an upsize to INV_X4.  The late b keeps g's A1 path
# from n passing, so side is the load of n that limits x.
create_clock -period 0.5 -name clk
set_input_delay -clock clk 0 [get_ports in]
set_input_delay -clock clk 0.3 [get_ports b]
set_output_delay -clock clk 0.3 [get_ports outg]
set_output_delay -clock clk 0.38 [get_ports outx]
set_output_delay -clock clk 0.47 [get_ports side]
set_load 40 [get_ports outg]
set_load 60 [get_ports outx]
set_load 30 [get_ports side]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Masters of x and g, the inserted buffers, and the slacks of x and side.
proc report_state { } {
  set buffers {}
  foreach inst [[ord::get_db_block] getInsts] {
    if { [$inst getName] ni {d x i1 i2 g} } {
      lappend buffers [[$inst getMaster] getName]
    }
  }
  puts "x: [get_property [get_cells x] ref_name]\
    g: [get_property [get_cells g] ref_name] buffers: [lsort $buffers]"
  puts [format "slack x/ZN: %.3f side: %.3f" \
    [get_property [get_pins x/ZN] slack_max] \
    [get_property [get_ports side] slack_max]]
  report_worst_slack -max -digits 3
  report_tns -digits 3
}

report_state

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_state

reset_opt_config -rebuffer_size_driver
