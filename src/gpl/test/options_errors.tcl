# The option checks are in C++, so Tcl reports the same messages as Python
# (see options_py_errors.py); the switch parsing adds its own.
source helpers.tcl
read_lef ./nangate45.lef
read_def ./simple01.def

foreach switches {
  {-densty 0.5}
  {-timing.nets_percentage abc}
  {-density 1.5}
  {-routability.rc_coefficients {1 2 3}}
  {-place_ios -skip_io}
  {-timing.repair_design.slew_margin 120}
  {-routability.global_route.grid_origin {1 2 3}}
  {-initial 5}
  {-density}
  {5}
  {-options {densty 0.5}}
  {-options {timing 5}}
  {-opt {density 0.5} -options}
} {
  catch { global_placement {*}$switches } msg
  puts $msg
}
catch { describe_options no_such_command } msg
puts $msg
