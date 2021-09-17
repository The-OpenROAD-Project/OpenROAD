# prim-dikstra with duplicate x or y locations
source "stt_helpers.tcl"

set alpha .4
set dup1 {dup1 0 {p0 0 0} {p1 10 10} {p2 10 20} {p3 10 10}}
report_pd_net $dup1 $alpha

set dup2 {dup2 2 {p0 29 43} {p1 28 45} {p2 28 44}}
report_pd_net $dup2 $alpha

set dup3 {dup3 2 {p0 100 56} {p1 65 37} {p2 65 38}}
report_pd_net $dup3 $alpha

# 2 duplicate points
set dup4 {dup4 0 {p0 10 10} {p1 10 10}}
report_pd_net $dup4 $alpha

set one {one 0 {p0 10 10}}
report_pd_net $one $alpha

# driver index changes by duplicate removal
set dup5 {dup5 2 {p0 123 209} {p1 123 209} {p2 123 215} {p3 122 211}}
report_pd_net $dup5 .3
