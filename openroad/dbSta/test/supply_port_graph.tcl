# Guard for #10414 and #8957 at the same time.
#
# dbNetwork hides a top-level BTerm from the pin and term walks only when the
# BTerm is signal-typed and merely sits on a supply net.  Two cases matter:
#
#   vccd1/vccd2/vssd1/vssd2  USE POWER/GROUND  -> real supply ports.  dbNetwork
#       reports power/ground direction for them, so Graph::makePinVertex()
#       never makes a vertex, and write_verilog needs them visible to emit the
#       "assign <port> = <net>;" alias when a port and its supply net have
#       different names (#10414).
#   bump1                    DIRECTION INOUT, no USE, on the vdpwr supply net
#       -> the bump-style port from #8957.  It is signal-typed, so its
#       direction is bidirect and exposing it would add two graph vertices
#       (load plus bidirect driver).  It must stay hidden.
#
# The pin walk and the term walk use the same predicate, so this test also
# pins down that the two views agree (no pin/term asymmetry).
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def supply_port_graph.def

# sta::vertex_iterator calls ensureGraph(), so this builds the timing graph.
# bump1 must not appear: it would contribute two vertices if it were exposed.
puts "--- timing graph vertices ---"
set vertex_names {}
set vertex_iter [sta::vertex_iterator]
while { [$vertex_iter has_next] } {
  set vertex [$vertex_iter next]
  lappend vertex_names [get_full_name [$vertex pin]]
}
$vertex_iter finish
foreach name [lsort $vertex_names] {
  puts "  $name"
}
puts "  vertex count: [llength $vertex_names]"

# Real supply ports are visible here; bump1 is not.
puts "--- top instance pin walk ---"
set top_pin_names {}
set pin_iter [[sta::top_instance] pin_iterator]
while { [$pin_iter has_next] } {
  lappend top_pin_names [get_full_name [$pin_iter next]]
}
$pin_iter finish
set top_pin_names [lsort $top_pin_names]
foreach name $top_pin_names {
  puts "  $name"
}

# The STA net-term walk (DbNetTermIterator) must agree with the pin walk.
puts "--- supply net term walk, cross-checked against the pin walk ---"
foreach net_name {vdpwr vgnd} {
  set net [sta::find_net $net_name]
  set term_names {}
  set term_iter [$net term_iterator]
  while { [$term_iter has_next] } {
    lappend term_names [get_full_name [[$term_iter next] pin]]
  }
  $term_iter finish
  set term_names [lsort $term_names]
  set only_in_terms {}
  foreach term_name $term_names {
    if { [lsearch -exact $top_pin_names $term_name] == -1 } {
      lappend only_in_terms $term_name
    }
  }
  puts "  $net_name terms: $term_names"
  puts "  $net_name terms not in the pin walk: $only_in_terms"
}

puts "--- write_verilog -include_pwr_gnd ---"
set verilog_file [make_result_file supply_port_graph.v]
write_verilog -include_pwr_gnd $verilog_file
report_file $verilog_file
