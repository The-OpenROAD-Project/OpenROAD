# Rerunning global_route -use_cugr in the same session: init() must rebuild
# the CUGR netlist from scratch (stale state used to fail with GRT-0076).
# The rerun must reproduce gcd_cugr's golden guides.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file global_route_rerun_cugr.guide]

global_route -verbose -use_cugr
global_route -verbose -use_cugr

write_guides $guide_file

diff_file gcd_cugr.guideok $guide_file
