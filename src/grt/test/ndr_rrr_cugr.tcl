# Validates that CUGR's iterative RRR loop (stage 4) preserves NDR
# scaling across rip-up / re-route cycles. We mirror cugr_adjustment3
# (gcd_nangate45 + 90% adjustment on every layer) which is the
# canonical design that forces RRR into stage 4, and attach an NDR
# to a handful of nets. The RRR loop calls removeTreeUsage(tree,
# costs) then commitTree(new_tree, false, costs) repeatedly; if any
# of those paths failed to honor the NDR factor, the net's demand
# would drift after each iteration and the final overflow / guide
# would diverge.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

# Wider-than-default NDR on a few signal nets that will get ripped
# up and re-routed by stage 4 under this 90% adjustment.
create_ndr -name NDR_2W \
  -spacing { metal2 0.20 metal3 0.20 metal4 0.40 metal5 0.40 metal6 0.40 } \
  -width { metal2 0.14 metal3 0.14 metal4 0.28 metal5 0.28 metal6 0.28 }

assign_ndr -ndr NDR_2W -net _100_
assign_ndr -ndr NDR_2W -net _200_
assign_ndr -ndr NDR_2W -net _300_
assign_ndr -ndr NDR_2W -net _400_

set guide_file [make_result_file ndr_rrr_cugr.guide]

set_global_routing_layer_adjustment metal1-metal10 0.9

global_route -verbose -use_cugr

write_guides $guide_file

diff_file ndr_rrr_cugr.guideok $guide_file
