source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file gcd_route.guide]

fastroute 

write_guides $guide_file

diff_file gcd.guideok $guide_file
