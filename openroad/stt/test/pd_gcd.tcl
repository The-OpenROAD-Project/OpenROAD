# prim-dikstra gcd
source "stt_helpers.tcl"

set alpha .8
set nets [read_nets "gcd.nets"]
#report_pd_net [find_net $nets req_rdy] $alpha
foreach net $nets {
  report_pd_net $net $alpha
}
