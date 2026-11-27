# A stack with backside layers: read_itf keeps the whole stack without a
# technology, and only the frontside once the technology marks the backside.
source "helpers.tcl"

read_itf backside.itf
add_field_solver_box -net b -layer BPR -rect {0 0 1 1}
clear_field_solver

read_lef backside.lef
read_itf backside.itf
check_error "a backside conductor is not in the stack" {
  add_field_solver_box -net b -layer BPR -rect {0 0 1 1}
} FSV-0007
check_error "a via to the backside is not in the stack" {
  add_field_solver_box -net b -layer VBPR -rect {0 0 0.1 0.1}
} FSV-0007

# The ground plane is the top of BPR, 0.1 um below M1. The layer's heights
# differ from the box's in the last bit, which changes some walks, so the
# check allows a few percent; a ground plane under BM1 changes the total far
# more.
add_field_solver_box -net m1 -layer M1 -rect {0 0 2 2}
set layer [solve_capacitance -min_walks 32768 -max_walks 32768]
clear_field_solver
add_field_solver_box -net m1 -box {0 0 0.1 2 2 0.2}
set box [solve_capacitance -min_walks 32768 -max_walks 32768]
check "M1 sits on the dielectric above the backside" {
  expr { abs([dict get $layer m1 total] / [dict get $box m1 total] - 1) < 0.05 }
} 1

clear_field_solver
add_field_solver_box -net v -layer V1 -rect {0 0 0.1 0.1}
check "a frontside via is kept" {
  dict keys [solve_capacitance -min_walks 32768 -max_walks 32768]
} v

exit_summary
