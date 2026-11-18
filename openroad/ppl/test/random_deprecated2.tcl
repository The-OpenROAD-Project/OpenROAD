# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -random_seed 20 -hor_layers metal3 -ver_layers metal2 \
  -corner_avoidance 0 -min_distance 0.12
