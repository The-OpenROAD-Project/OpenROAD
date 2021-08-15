# prim-dikstra II gcd
source "stt_helpers.tcl"

set alpha .8
set nets [read_nets "gcd.nets"]
#report_pdrev_net [lindex $nets 0] $alpha
foreach net $nets {
  report_pdrev_net $net $alpha
}
