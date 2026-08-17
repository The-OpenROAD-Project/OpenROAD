# Ownership is decided by extraction rate, so a claim that no longer holds does
# not by itself sink the verdict, and a claim the embedder skipped is not
# counted at all.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

# 5 checkable claims, 4 of which hold: above the default threshold.
puts "verdict [verify_watermark -placement_claims wm_place_claims.csv -min_stages 1]"

# The same evidence is not enough once the bar is raised past it.
puts "strict verdict [verify_watermark -placement_claims wm_place_claims.csv -tau 0.9 -min_stages 1]"
