# check that CUGR leaves out nets skipped by -skip_large_fanout_nets.
# -resistance_aware runs the timing update, which estimates parasitics
# for every net CUGR routed.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_def "gcd.def"

set guide_file [make_result_file skip_large_fanout1_cugr.guide]

global_route -verbose -use_cugr -resistance_aware -skip_large_fanout_nets 30

write_guides $guide_file

diff_files skip_large_fanout1_cugr.guideok $guide_file
