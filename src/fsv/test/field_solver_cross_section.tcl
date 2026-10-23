# A SkyWater 130 met1 pair solved as a cross section, against the 2D
# finite-volume reference of golden/sky130_met1_pair (fF/um).
source "helpers.tcl"

read_itf sky130.itf
add_field_solver_rect -net a -layer met1 -x {0 0.14}
add_field_solver_rect -net b -layer met1 -x {0.28 0.42}
set result [solve_capacitance -nets a -tol_total 0.005]

check "the row is per unit length" { dict get $result a cross_section } 1
check "the total matches the reference" {
  expr { abs([dict get $result a total] / 0.188499 - 1) < 0.02 }
} 1
check "the coupling matches the reference" {
  expr { abs([dict get $result a couplings b] / 0.141776 - 1) < 0.03 }
} 1

# Rectangles by their corners, in a uniform dielectric: the matrix is
# symmetric within the statistical error.
clear_field_solver
set_field_solver_permittivity 3.9
add_field_solver_rect -net a -rect {0 0.5 0.2 0.7}
add_field_solver_rect -net b -rect {0.4 0.5 0.6 0.7}
set uniform [solve_capacitance -tol_total 0.01]
set sigma [expr {
  hypot([dict get $uniform a coupling_sigmas b],
    [dict get $uniform b coupling_sigmas a])
}]
check "couplings are reciprocal" {
  expr {
    abs([dict get $uniform a couplings b] - [dict get $uniform b couplings a])
    < 4 * $sigma
  }
} 1

exit_summary
