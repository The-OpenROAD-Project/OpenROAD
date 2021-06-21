# prim-dikstra with duplicate x or y locations
source "pdrev_helpers.tcl"

set alpha .8
set dup1 {dup1 0 {p0 0 0} {p1 10 10} {p2 10 20} {p3 10 10}}
report_pdrev_net $dup1 $alpha 1
report_pdrev_net $dup1 $alpha 0

set dup2 {dup2 2 {p0 29 43} {p1 28 45} {p2 28 44}}
report_pdrev_net $dup2 $alpha 1

set dup3 {dup3 2 {p0 100 56} {p1 65 37} {p2 65 38}}
report_pdrev_net $dup3 $alpha 1
