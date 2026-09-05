# Companion to place.tcl: the same claims, checked against the design as it
# was before embedding.
#
# This is the test that gives the extraction rate its meaning.  Verification
# compares a claimed order against the design, and a claim is a coin flip on a
# design the key never marked -- so an unmarked design has to land near one
# half, well under the ownership threshold.  If it lands at one instead, the
# embedder chose its claims after seeing which pairs already happened to match,
# and the rate is measuring nothing: any key would score full marks on any
# design, including one its holder had never touched.
#
# wm_place_marked.csv is what place.tcl commits.  Seven of its twenty pairs
# were already in the keyed order and needed no swap; those are exactly the
# seven that hold here.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

check "an unmarked design does not carry the mark" \
  { verify_watermark -placement_claims wm_place_marked.csv -min_stages 1 } 0

exit_summary
