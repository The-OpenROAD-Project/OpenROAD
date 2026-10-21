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
  fsv::set_permittivity_cmd 0
} FSV-0004

read_itf sky130.itf
check_error "an unknown layer" {
  add_field_solver_box -net a -layer met9 -rect {0 0 1 1}
} FSV-0007
add_field_solver_box -net a -layer met1 -rect {0 0 1 0.14}
check_error "an unknown net" { solve_capacitance -nets b } FSV-0009
check_error "an unreadable ITF" { read_itf missing.itf } FSV-0001

exit_summary
