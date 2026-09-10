# Invalid public inputs must fail before changing router configuration.
source "helpers.tcl"
set_routing_watermark_strength 100
foreach strength { -1 bogus Inf -Inf NaN 1e20 4294967296 } {
  check "reject invalid strength $strength" {
    catch {set_routing_watermark_strength $strength}
  } 1
  check "invalid strength preserves configuration" { get_routing_watermark_strength } 100.0
}
# Also check the SWIG entry point: Tcl argument checks cannot be the only guard.
foreach strength { -1 Inf 1e20 } {
  check "SWIG rejects invalid strength $strength" {
    catch {drt::set_routing_watermark_strength_cmd $strength}
  } 1
  check "invalid SWIG input preserves configuration" { get_routing_watermark_strength } 100.0
}
foreach strength { 0 0.5 1 100 4294967040 } {
  set_routing_watermark_strength $strength
  check "representable strength round-trips" {
    expr {[get_routing_watermark_strength] == $strength}
  } 1
}
exit_summary
