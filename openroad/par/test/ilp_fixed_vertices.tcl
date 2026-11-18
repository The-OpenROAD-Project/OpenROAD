# Verify that ILP partitioning honors fixed vertex assignments.
set test_name ilp_fixed_vertices
source helpers.tcl

# Partitioning writes beside the hypergraph. Copy it to the test result
# directory so running the test does not overwrite a tracked input.
set graph [make_result_file "${test_name}.hgr"]
file copy -force "${test_name}.hgr" $graph

triton_part_hypergraph \
  -hypergraph_file $graph \
  -fixed_file "${test_name}.fix" \
  -num_parts 2 \
  -balance_constraint 2 \
  -seed 0 \
  -vertex_dimension 1 \
  -hyperedge_dimension 1

set fixed_stream [open "${test_name}.fix" r]
set fixed [split [string trim [read $fixed_stream]] "\n"]
close $fixed_stream

set part_stream [open "${graph}.part.2" r]
set parts [split [string trim [read $part_stream]] "\n"]
close $part_stream

if { [llength $parts] != [llength $fixed] } {
  error "Result contains [llength $parts] vertices; expected [llength $fixed]"
}

set fixed_count 0
for { set v 0 } { $v < [llength $fixed] } { incr v } {
  set required [lindex $fixed $v]
  if { $required >= 0 } {
    incr fixed_count
    set actual [lindex $parts $v]
    if { $actual != $required } {
      error "Vertex [expr { $v + 1 }]: expected part $required, got $actual"
    }
  }
}

if { $fixed_count != 4 } {
  error "Expected 4 fixed vertices; found $fixed_count"
}
puts "pass"
