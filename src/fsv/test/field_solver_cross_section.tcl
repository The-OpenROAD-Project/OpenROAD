# A SkyWater 130 met1 pair solved as a cross section, against the 2D
# finite-volume reference of golden/sky130_met1_pair (fF/um).
source "helpers.tcl"

read_itf sky130.itf
add_field_solver_box -net a -layer met1 -rect {0 0 1 0.14}
add_field_solver_box -net b -layer met1 -rect {0 0.28 1 0.42}
set result [solve_capacitance -cross_section -nets a -tol_total 0.005]

check "the row is per unit length" { dict get $result a cross_section } 1
check "the total matches the reference" {
  expr { abs([dict get $result a total] / 0.188499 - 1) < 0.02 }
} 1
check "the coupling matches the reference" {
  expr { abs([dict get $result a couplings b] / 0.141776 - 1) < 0.03 }
} 1

# The x extents do not matter.
clear_field_solver
add_field_solver_box -net a -layer met1 -rect {-5 0 5 0.14}
add_field_solver_box -net b -layer met1 -rect {-5 0.28 5 0.42}
set longer [solve_capacitance -cross_section -nets a -tol_total 0.005]
check "a cross section ignores x" {
  expr { [dict get $longer a total] == [dict get $result a total] }
} 1

exit_summary
