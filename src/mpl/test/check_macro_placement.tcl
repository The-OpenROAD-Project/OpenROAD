# Smoke-test the macro preflight binding and its argument translation.
source "helpers.tcl"

check_error "missing design" { check_macro_placement } MPL-0001

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"
read_def "./testcases/fixed_macros1.def"

set reports_before [glob -nocomplain hier_rtlmp]
check "preflight with explicit channels and fence" {
  check_macro_placement -min_channel_size {0.2 0.4} -pin_aware_channels \
    -fence_lx 0 -fence_ly 0 -fence_ux 220 -fence_uy 221.2
} true
check "preflight with scalar channel" {
  check_macro_placement -min_channel_size 0.2
} true
check "default preflight" { check_macro_placement } true
check "no report directory created" {
  glob -nocomplain hier_rtlmp
} $reports_before
check_error "channel cardinality" {
  check_macro_placement -min_channel_size {1 2 3}
} MPL-0080
check_error "pin-aware channel requirement" {
  check_macro_placement -pin_aware_channels
} MPL-0079

exit_summary
