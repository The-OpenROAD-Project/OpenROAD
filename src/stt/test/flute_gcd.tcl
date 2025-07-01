# flute gcd
source "stt_helpers.tcl"

set nets [read_nets "gcd.nets"]
foreach net $nets {
  report_flute_net $net
}
