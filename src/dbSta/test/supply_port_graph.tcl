# Guard for #10414 and #8957 at the same time.
#
# DbInstancePinIterator exposes the top block's power/ground BTerms (needed so
# write_verilog can emit the "assign <port> = <net>;" aliases when a supply port
# and its net have different names -- #10414).  Graph::makeVerticesAndEdges()
# walks that same iterator, so this test pins down that the supply terminals
# still do NOT become timing-graph vertices (the growth #8957 fixed).
#
# It also cross-checks the top instance pin walk against the supply nets' pin
# and term walks, so the golden records which iterators expose supply
# terminals and any future change to that shows up here.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def write_verilog10.def

# sta::vertex_iterator calls ensureGraph(), so this builds the timing graph.
puts "--- timing graph vertices (supply pins must not appear) ---"
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

puts "--- top instance pins (supply ports are visible here) ---"
set top_pin_names {}
set pin_iter [[sta::top_instance] pin_iterator]
while { [$pin_iter has_next] } {
  lappend top_pin_names [get_full_name [$pin_iter next]]
}
$pin_iter finish
foreach name [lsort $top_pin_names] {
  puts "  $name"
}

puts "--- cross-check: supply BTerms on each supply net (odb) vs the pin walk ---"
set block [ord::get_db_block]
foreach net_name {vdpwr vgnd} {
  set db_net [$block findNet $net_name]
  set net_bterms {}
  foreach bterm [$db_net getBTerms] {
    lappend net_bterms [$bterm getName]
  }
  set net_bterms [lsort $net_bterms]
  # Every BTerm odb reports on the supply net must also be reachable from the
  # top instance pin walk; that is what write_verilog uses to emit the alias.
  set missing {}
  foreach bterm_name $net_bterms {
    if { [lsearch -exact $top_pin_names $bterm_name] == -1 } {
      lappend missing $bterm_name
    }
  }
  puts "  $net_name bterms: $net_bterms"
  puts "  $net_name missing from pin walk: $missing"
}

puts "--- write_verilog -include_pwr_gnd (assign aliases must stay) ---"
set verilog_file [make_result_file supply_port_graph.v]
write_verilog -include_pwr_gnd $verilog_file
report_file $verilog_file
