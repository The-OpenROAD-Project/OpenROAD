# The option checks are in C++, so Tcl reports the same messages as Python
# (see options_errors.py).
source helpers.tcl
read_lef ./nangate45.lef
read_def ./simple01.def

foreach opts {
  {densty 0.5}
  {timing {nets_percentage abc}}
  {density 1.5}
  {routability.rc_coefficients {1 2 3}}
  {place_ios true skip_io true}
  {timing {repair_design {slew_margin 120}}}
  {routability.global_route.grid_origin {1 2 3}}
  {initial 5}
} {
  catch { global_placement -options $opts } msg
  puts $msg
}
catch { global_placement -density 1.5 } msg
puts $msg
catch { describe_options no_such_command } msg
puts $msg
