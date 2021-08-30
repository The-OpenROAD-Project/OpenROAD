# Challenging nets from gf/swerv_wrapper with segment overlaps.
source "stt_helpers.tcl"

# squack when segment overlap is detected
set_debug_level STT check 1

# set overlap1 {overlap1 8
#   {p0 11700 2700}
#   {p1 8900 2500}
#   {p2 11800 3000}
#   {p3 11600 2700}
#   {p4 5200 2300}
#   {p5 5000 2000}
#   {p6 5000 2100}
#   {p7 5200 1400}
#   {p8 12200 1600}
#   {p9 11700 2700}
#   {p10 8900 2500}
#   {p11 11600 2700}
#   {p12 5200 2300}
#   {p13 5000 2100}
#   {p14 11700 2700}
#   {p15 5200 2300}
# }

# report_stt_net $overlap1 0.3
