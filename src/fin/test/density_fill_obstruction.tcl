# Check that density fill honors only obstructions marked for fills.
source "helpers.tcl"

proc count_fill_overlaps { block layer region } {
  set count 0
  foreach fill [$block getFills] {
    if { [$fill getTechLayer] == $layer } {
      set rect [$fill getRect]
      if {
        [$rect xMin] < [$region xMax]
        && [$rect xMax] > [$region xMin]
        && [$rect yMin] < [$region yMax]
        && [$rect yMax] > [$region yMin]
      } {
        incr count
      }
    }
  }
  return $count
}

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def gcd_prefill.def

set block [ord::get_db_block]
set layer [[ord::get_db_tech] findLayer met2]

set obstruction [create_obstruction -layer met2 -region {258 10 263 18} -fill]
set fill_obstruction [$obstruction getBBox]

set obstruction [create_obstruction -layer met2 -region {258 30 263 38}]
set routing_obstruction [$obstruction getBBox]

density_fill -rules fill.json

set fill_overlaps [count_fill_overlaps $block $layer $fill_obstruction]
if { $fill_overlaps != 0 } {
  error "$fill_overlaps fills overlap a fill obstruction"
}

set routing_overlaps [count_fill_overlaps $block $layer $routing_obstruction]
if { $routing_overlaps == 0 } {
  error "routing obstruction unexpectedly blocks fills"
}

puts "pass"
