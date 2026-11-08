# Wires and a via on SkyWater 130 layers, placed by layer name.
source "helpers.tcl"

read_itf sky130.itf
add_field_solver_box -net m1a -layer met1 -rect {0 0 10 0.14}
add_field_solver_box -net m1b -layer met1 -rect {0 0.28 10 0.42}
add_field_solver_box -net m2 -layer met2 -rect {4.5 -3 4.64 3.42}
set result [solve_capacitance -nets m1a -tol_total 0.02]

check "only the requested net is solved" { dict keys $result } m1a
check "the neighbour couples more than the crossing wire" {
  expr { [dict get $result m1a couplings m1b] > [dict get $result m1a couplings m2] }
} 1

# A via alone over the ground plane.
clear_field_solver
add_field_solver_box -net v -layer via -rect {0 0 0.15 0.15}
set via [solve_capacitance -tol_total 0.02]
check "a lone via couples only to ground" {
  expr { abs([dict get $via v ground] - [dict get $via v total])
         < 3 * [dict get $via v total_sigma] }
} 1

# The via spans from the top of met1 to the bottom of met2. These heights
# differ from the stack's sums in the last bit, which changes some walks, so
# the check allows a few percent; a wrong extent changes the total far more.
clear_field_solver
add_field_solver_box -net v -box {0 0 1.6611 0.15 0.15 1.9311}
set box [solve_capacitance -tol_total 0.02]
check "the via layer gives the via's z extent" {
  expr { abs([dict get $box v total] / [dict get $via v total] - 1) < 0.05 }
} 1

# A met1/met2 grid: 20 sidewall shells, more than the dielectric scans
# directly, so cubes find nearby shells through its index. The walk count
# is fixed and the log records the result exactly.
clear_field_solver
for { set i 0 } { $i < 10 } { incr i } {
  set y [expr { 0.34 * $i }]
  add_field_solver_box -net h$i -layer met1 -rect [list 0 $y 4 [expr { $y + 0.14 }]]
  set x [expr { 0.4 * $i }]
  add_field_solver_box -net v$i -layer met2 -rect [list $x -0.5 [expr { $x + 0.14 }] 3.5]
}
set grid [solve_capacitance -nets h4 -min_walks 32768 -max_walks 32768]
check "the grid's middle wire couples most to its neighbours" {
  expr { [dict get $grid h4 couplings h3] > [dict get $grid h4 couplings v4] }
} 1

exit_summary
