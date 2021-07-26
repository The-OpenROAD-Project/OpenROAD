# prim-dikstra gcd
source "pdrev_helpers.tcl"

set alpha .8
set nets [read_nets "gcd.nets"]
report_pdrev_nets $nets $alpha 1
#report_pdrev_net [find_pdrev_net $nets req_rdy] $alpha 1
