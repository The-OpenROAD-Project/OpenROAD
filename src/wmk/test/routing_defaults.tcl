# Embedding and verification must select the same nets when fractions are
# omitted. Synthetic wires isolate command defaults from router heuristics.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set block [ord::get_db_block]
set layer [[ord::get_db_tech] findLayer metal1]
set key [string repeat 0 64]
set nets {}
for { set i 0 } { $i < 100 } { incr i } {
  lappend nets [odb::dbNet_create $block probe_$i]
}

set encoder [odb::dbWireEncoder]
foreach { label embed_options verify_options } {
  default {} {}
  explicit {-fraction 0.5} {-routing_fraction 0.5}
} {
  set count [set_routing_watermark -key_hex $key {*}$embed_options]
  # Only these 100 nets have wires, so they form the verification population.
  # Give marked nets preferred-direction wires and all others wrong-way wires.
  foreach net $nets {
    set wire [$net getWire]
    if { $wire eq "NULL" } {
      set wire [odb::dbWire_create $net]
    }
    $encoder begin $wire
    $encoder newPath $layer ROUTED
    $encoder addPoint 0 0
    if { [odb::dbBoolProperty_find $net watermark] ne "NULL" } {
      $encoder addPoint 1000 0
    } else {
      $encoder addPoint 0 1000
    }
    $encoder end
  }
  check "$label routing tags can be removed" { clear_routing_watermark } $count
  if { $label eq "default" } {
    check "default embedding uses the documented fraction" {
      verify_watermark -routing_key_hex $key -routing_fraction 0.05 -min_stages 1
    } 1
  }
  check "$label fractions recover the routing watermark without tags" {
    verify_watermark -routing_key_hex $key {*}$verify_options -min_stages 1
  } 1
}
exit_summary
