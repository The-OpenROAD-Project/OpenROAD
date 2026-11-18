# Invalid public inputs must fail before changing router configuration, and
# the multiplier round-trips through it.
source "helpers.tcl"
check "the default strength is the documented one" { get_routing_watermark_strength } 100.0
set_routing_watermark_strength 100
foreach strength { -1 bogus Inf -Inf NaN 1e20 10001 4294967040 4294967296 } {
  check "reject invalid strength $strength" {
    catch {set_routing_watermark_strength $strength}
  } 1
  check "invalid strength preserves configuration" { get_routing_watermark_strength } 100.0
}
# Also check the SWIG entry point: Tcl argument checks cannot be the only guard.
foreach strength { -1 Inf 1e20 10001 } {
  check "SWIG rejects invalid strength $strength" {
    catch {drt::set_routing_watermark_strength_cmd $strength}
  } 1
  check "invalid SWIG input preserves configuration" { get_routing_watermark_strength } 100.0
}
foreach strength { 0 0.5 1 8 100 10000 } {
  set_routing_watermark_strength $strength
  check "representable strength round-trips" {
    expr {[get_routing_watermark_strength] == $strength}
  } 1
}
exit_summary
