# CUGR with via congestion demand disabled through the developer switch
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

grt::set_cugr_via_demand 0

global_route -verbose -use_cugr
