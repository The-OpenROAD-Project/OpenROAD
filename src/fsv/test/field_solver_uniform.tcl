# Two parallel wires in a uniform dielectric: the full capacitance matrix.
source "helpers.tcl"

set_field_solver_permittivity 3.9
add_field_solver_box -net a -box {0 0 0.5 5 0.2 0.7}
add_field_solver_box -net b -box {0 0.4 0.5 5 0.6 0.7}
set result [solve_capacitance -tol_total 0.01]

check "every net is solved" { dict keys $result } {a b}
set keys {total total_sigma ground ground_sigma couplings coupling_sigmas
  walks hops seconds cross_section window_leakage}
check "a row's keys" { dict keys [dict get $result a] } [list {*}$keys]
check "couplings name the other nets" { dict keys [dict get $result a couplings] } b

# The matrix is symmetric within the statistical error.
set c_ab [dict get $result a couplings b]
set c_ba [dict get $result b couplings a]
set sigma [expr {
  hypot([dict get $result a coupling_sigmas b],
    [dict get $result b coupling_sigmas a])
}]
check "couplings are reciprocal" { expr { abs($c_ab - $c_ba) < 4 * $sigma } } 1

# The permittivity scales every entry.
set_field_solver_permittivity 7.8
set scaled [solve_capacitance -nets a -tol_total 0.01]
check "permittivity scales the total" {
  expr { abs([dict get $scaled a total] / [dict get $result a total] - 2) < 0.01 }
} 1

exit_summary
