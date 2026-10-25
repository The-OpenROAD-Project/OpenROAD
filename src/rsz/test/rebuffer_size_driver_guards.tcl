# fully_rebuffer with set_opt_config -rebuffer_size_driver leaves fixed and
# sequential drivers at their current cell.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
create_clock [get_ports clk] -name core_clock -period 0.5
set_max_fanout 100 [current_design]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# _488_ and _490_ are upsized (replacing a buffer) when they are not fixed.
set block [ord::get_db_block]
foreach name {_488_ _490_} {
  [$block findInst $name] setPlacementStatus FIRM
}
set flop_masters {}
foreach inst [$block getInsts] {
  if { [string match "DFF*" [[$inst getMaster] getName]] } {
    dict set flop_masters [$inst getName] [[$inst getMaster] getName]
  }
}

set_opt_config -rebuffer_size_driver true
report_opt_config
rsz::fully_rebuffer NULL

foreach name {_488_ _490_} {
  set inst [$block findInst $name]
  puts "$name [[$inst getMaster] getName] [$inst getPlacementStatus]"
}
set resized_flops 0
dict for {name master} $flop_masters {
  if { [[[$block findInst $name] getMaster] getName] ne $master } {
    incr resized_flops
  }
}
puts "flops: [dict size $flop_masters] resized: $resized_flops"

reset_opt_config -rebuffer_size_driver
