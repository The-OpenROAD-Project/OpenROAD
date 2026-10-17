# Roundoff in a large tied population must not produce ownership evidence.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
set fixture [make_result_file routing_ties.def]
set stream [open $fixture w]
puts $stream {VERSION 5.8 ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;
DESIGN routing_ties ;
UNITS DISTANCE MICRONS 2000 ;
DIEAREA ( 0 0 ) ( 1600000 800000 ) ;
END DESIGN}
close $stream
read_def $fixture

set block [ord::get_db_block]
set layer [[ord::get_db_tech] findLayer metal1]
set encoder [odb::dbWireEncoder]
set nets {}
for { set i 0 } { $i < 5000 } { incr i } {
  lappend nets [odb::dbNet_create $block tied_probe_$i]
}

# No embedding or biased routing. These keys omit the exceptional net in the
# mixed case, whose exact null tail is (5000-marked)/5000, greater than 0.95.
foreach mode { uniform mixed } {
  set i 0
  foreach net $nets {
    set wire [$net getWire]
    if { $wire eq "NULL" } {
      set wire [odb::dbWire_create $net]
    }
    set preferred 1000
    set wrongway 9000
    if { $mode eq "mixed" } {
      set preferred [expr { $i == 4999 ? 1000 : 2000 }]
      set wrongway [expr { $i == 4999 ? 2000 : 1000 }]
    }
    set x [expr { ($i % 100) * 16000 }]
    set y [expr { ($i / 100) * 16000 }]
    $encoder begin $wire
    $encoder newPath $layer ROUTED
    $encoder addPoint $x $y
    $encoder addPoint [expr { $x + $preferred }] $y
    $encoder addPoint [expr { $x + $preferred }] [expr { $y + $wrongway }]
    $encoder end
    incr i
  }
  foreach digit { 0 1 2 3 } {
    set key [string repeat $digit 64]
    check "$mode population does not establish ownership with key $digit" {
      verify_watermark -routing_key_hex $key -min_stages 1
    } 0
  }
}
exit_summary
