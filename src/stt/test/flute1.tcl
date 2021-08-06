# flute with duplicate x or y locations and 1-pin net
source "pdrev_helpers.tcl"

set dup1 {dup1 0 {p0 0 0} {p1 10 10} {p2 10 20} {p3 10 10}}
report_flute_net $dup1
report_flute_net $dup1

set dup2 {dup2 2 {p0 29 43} {p1 28 45} {p2 28 44}}
report_flute_net $dup2

set dup3 {dup3 2 {p0 100 56} {p1 65 37} {p2 65 38}}
report_flute_net $dup3

# 2 duplicate points
set dup4 {dup4 0 {p0 10 10} {p1 10 10}}
report_flute_net $dup4

set one {one 0 {p0 10 10}}
report_flute_net $one
