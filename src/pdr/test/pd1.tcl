# prim-dikstra with duplicate xy locations
source "pdrev_helpers.tcl"

set alpha .8
set net {dup1 4 0 {{p1 0 0} {p2 10 10} {p2 10 20} {p3 10 10}}}
report_pdrev_net $net $alpha 1
report_pdrev_net $net $alpha 0
