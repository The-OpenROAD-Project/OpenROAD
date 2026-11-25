# fully_rebuffer with set_opt_config -rebuffer_size_driver on two passing
# nets driven by INVx1 RVT (ASAP7 RVT/LVT/SLVT loaded).  A same-area VT swap
# gains slack the nets do not need, so both drivers must stay RVT.
# Net a: the timing tree adds a leaky buffer that area recovery removes; the
# leakage budget must come from the buffer-free baseline tree, not from the
# timing tree.  Net b: the baseline tree keeps buffers, so the budget allows
# SLVT; the swap must still not raise net leakage above the baseline.
source "helpers.tcl"
foreach vt {RVT LVT SLVT} {
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
}
read_lef asap7/asap7_tech_1x_201209.lef
foreach vt {R L SL} {
  read_lef asap7/asap7sc7p5t_28_${vt}_1x_220121a.lef
}

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer M1]
set inv [$db findMaster INVx1_ASAP7_75t_R]

# Placed top-level port on net at (x, y) um.
proc make_port { net name io x y } {
  global pin_layer
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
}

# Inverter x<tag> fed by port in_<tag> drives `count` output ports `dist` um
# away.
proc make_net { tag count dist y } {
  global block inv
  set in_net [odb::dbNet_create $block in_$tag]
  set out_net [odb::dbNet_create $block n_$tag]
  make_port $in_net in_$tag INPUT 0 $y
  set inst [odb::dbInst_create $block $inv x$tag]
  [$inst findITerm A] connect $in_net
  [$inst findITerm Y] connect $out_net
  $inst setLocation [ord::microns_to_dbu 2] [ord::microns_to_dbu $y]
  $inst setPlacementStatus PLACED
  for { set i 0 } { $i < $count } { incr i } {
    make_port $out_net out_$tag$i OUTPUT [expr { $dist + 2 * $i }] $y
  }
}

make_net a 4 60 0
make_net b 1 500 100
ord::design_created

set_dont_use {*xp*_ASAP7*}
create_clock -period 1000 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [all_outputs]
set_load 1 [all_outputs]
source asap7/setRC.tcl
estimate_parasitics -placement

# Driver master and driver pin slack of each net, and the inserted buffers.
proc report_nets { } {
  foreach tag {a b} {
    set master [get_property [get_cells x$tag] ref_name]
    set slack [get_property [get_pins x$tag/Y] slack_max]
    puts "x$tag: $master slack: [format %.2f $slack]"
  }
  set buffers {}
  foreach inst [get_cells -quiet place*] {
    lappend buffers [get_property $inst ref_name]
  }
  puts "buffers: [lsort $buffers]"
}

report_nets

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_nets
report_worst_slack -max

reset_opt_config -rebuffer_size_driver
