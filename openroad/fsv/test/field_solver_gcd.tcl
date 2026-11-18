# Nets of sky130hd gcd, routed by OpenROAD-flow-scripts and trimmed to the
# conductors near three nets (see README.md): a short met1 net, a net with
# jogs and vias up to met3, and an input net ending at a block pin on the
# die's edge.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def field_solver_gcd.def
read_itf sky130.itf

foreach net {_042_ _119_/_067_ req_msg[10]} {
  add_field_solver_nets -nets $net -halo 4 -margin 1
  set row [dict get [solve_capacitance -min_walks 65536 -max_walks 65536] $net]
  check "$net couples to its neighbours" {
    expr { [dict size [dict get $row couplings]] > 0 }
  } 1
}

# The leakage falls as the window grows: req_msg[10] reaches met3, so a
# 4 um halo is only about one height.
set leakage {}
foreach halo {2 6} {
  add_field_solver_nets -nets req_msg[10] -halo $halo -margin 1
  set row [dict get [solve_capacitance -min_walks 65536 -max_walks 65536] \
    req_msg[10]]
  lappend leakage [dict get $row window_leakage]
}
check "a wider window leaks less" {
  expr { [lindex $leakage 1] < [lindex $leakage 0] }
} 1

exit_summary
