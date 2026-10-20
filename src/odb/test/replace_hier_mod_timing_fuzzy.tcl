# Hierarchical timing assertions must share STA's relative and zero comparisons.
set test_name replace_hier_mod_timing_fuzzy
source hierarchy_timing_helpers.tcl

foreach case {
  {-2.0000005e-8 -2e-8 0}
  {-2.000004e-8 -2e-8 1}
  {-1.99999e-8 -2e-8 0}
  {2e-8 2.0000005e-8 0}
  {1.999996e-8 2e-8 1}
  {-0.5e-15 0 0}
  {-1e-15 0 1}
  {-2e-15 0 1}
  {0 0.5e-15 0}
  {0 1e-15 1}
  {0 2e-15 1}
  {0.5e-20 1e-20 1}
  {-1e-20 1e-20 1}
  {0 0 0}
  {-2e-8 -2e-8 0}
} {
  lassign $case actual expected degraded
  if { [hierarchy_timing_less $actual $expected] != $degraded } {
    error "Timing degradation disagrees with STA for $actual vs $expected"
  }
}

# A difference larger than 1 fs is still equal under the relative tolerance.
hierarchy_timing_equal -2.0000005e-8 -2e-8 "Relative-boundary setup TNS"
hierarchy_timing_equal -0.5e-15 0 "Zero-boundary setup WS"
if { ![catch { hierarchy_timing_equal -2.000004e-8 -2e-8 "Outside tolerance" }] } {
  error "Timing equality failed to reject a difference beyond STA's relative tolerance"
}
if { ![catch { hierarchy_timing_equal -1e-15 0 "At the zero threshold" }] } {
  error "Timing equality failed to preserve STA's strict zero threshold"
}
if { ![catch { hierarchy_timing_equal 0.5e-20 1e-20 "Both nonzero" }] } {
  error "Nonzero timing values must not use an absolute 1 fs tolerance floor"
}
puts pass
