# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file silence.guide]

global_route

write_guides $guide_file

diff_file silence.guideok $guide_file
