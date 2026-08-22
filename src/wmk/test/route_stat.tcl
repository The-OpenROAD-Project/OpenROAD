# A routed design that carries no watermark must not be claimed by one.
#
# 45_gcd.def is an ordinary routed design.  The first key below is chosen
# because its marked nets happen to carry less wrong-way metal than the rest --
# T_R comes out negative, which is the direction a real watermark points.  It
# is a coincidence, and the stage has to say so: on an unwatermarked design the
# sign of T_R is a coin flip, so judging on the sign alone would hand ownership
# to whichever of two keys got lucky.  What decides is how improbable the value
# is, and 1.3e-02 is not improbable.  The second key is the ordinary case.
#
# The counts are pinned as well.  Wrong-way fraction is measured on
# canonicalized geometry -- collinear and overlapping records merged per layer
# and line, vias excluded -- because a router may split one straight wire into
# several records without moving any metal, and a statistic that counted
# records instead of length would move when the route did not.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45.def

set tempting 5555555555555555555555555555555555555555555555555555555555555555
set ordinary 3333333333333333333333333333333333333333333333333333333333333333

check "a lucky key is not ownership" \
  { verify_watermark -routing_key_hex $tempting -routing_fraction 0.30 -min_stages 1 } 0
check "nor is an ordinary one" \
  { verify_watermark -routing_key_hex $ordinary -routing_fraction 0.30 -min_stages 1 } 0

exit_summary
