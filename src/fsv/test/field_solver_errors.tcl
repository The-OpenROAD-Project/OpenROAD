# Argument and state errors of the field-solver commands.
source "helpers.tcl"

check_error "nothing to solve" { solve_capacitance } FSV-0023
check_error "a layer needs a stack" {
  add_field_solver_box -net a -layer met1 -rect {0 0 1 1}
} FSV-0006
check_error "a box needs six numbers" {
  add_field_solver_box -net a -box {0 0 1 1}
} FSV-0024
check_error "a box or a layer is needed" { add_field_solver_box -net a } FSV-0022
check_error "-box excludes -layer" {
  add_field_solver_box -net a -box {0 0 1 1 1 2} -layer met1
} FSV-0021
check_error "a box below the ground plane" {
  add_field_solver_box -net a -box {0 0 -1 1 1 1}
} FSV-0005
check_error "a permittivity must be positive" {
  set_field_solver_permittivity 0
} FSV-0004

read_itf sky130.itf
check_error "an unknown layer" {
  add_field_solver_box -net a -layer met9 -rect {0 0 1 1}
} FSV-0007
add_field_solver_box -net a -layer met1 -rect {0 0 1 0.14}
check_error "an unknown net" { solve_capacitance -nets b } FSV-0009
check_error "a tolerance must be positive" {
  solve_capacitance -tol_total 0
} FSV-0025
check_error "a solve needs a walk" { solve_capacitance -max_walks 0 } FSV-0027
check_error "panels come in multiples of 16" {
  solve_capacitance -panels 20
} FSV-0028
# Below the command's own check, the solver's error is reported once.
check_error "the solver rejects a panel count" {
  fsv::solve_net_cmd a -1 -1 -1 -1 -1 20
} FSV-0012
set result [solve_capacitance -seed 3000000000 -tol_total 0.05]
check "a seed beyond 32 bits is accepted" { dict exists $result a total } 1
check_error "rectangles after boxes" {
  add_field_solver_rect -net b -rect {0 0.5 1 0.7}
} FSV-0015
clear_field_solver
check_error "-rect excludes -layer" {
  add_field_solver_rect -net a -rect {0 0.5 1 0.7} -layer met1
} FSV-0016
check_error "a rectangle needs -rect or -layer and -x" {
  add_field_solver_rect -net a -layer met1
} FSV-0017
check_error "a span has two numbers" {
  add_field_solver_rect -net a -layer met1 -x {0 1 2}
} FSV-0024
check_error "a rectangle below the ground plane" {
  add_field_solver_rect -net a -rect {0 -1 1 0.5}
} FSV-0005
check_error "a rectangle needs a net" {
  add_field_solver_rect -rect {0 0.5 1 0.7}
} FSV-0020
add_field_solver_rect -net a -rect {0 0.5 1 0.7}
check_error "boxes after rectangles" {
  add_field_solver_box -net b -box {0 0 0.5 1 1 0.7}
} FSV-0015
add_field_solver_rect -net b -rect {0.5 0.6 1.5 0.8}
check_error "rectangles of different nets overlap" {
  solve_capacitance -nets a
} FSV-0008
clear_field_solver
check_error "an unreadable ITF" { read_itf missing.itf } FSV-0001

read_itf substrate_via.itf
check_error "a via on the ground plane" {
  add_field_solver_box -net t -layer tie -rect {0 0 0.1 0.1}
} FSV-0014
check_error "a capacitive etch the solver does not apply" {
  read_itf capacitive_etch.itf
} FSV-0019

exit_summary
