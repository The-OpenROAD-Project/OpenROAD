# CUGR via-demand geometry model: the per-layer-pair via footprints are derived
# from the tech's real via enclosures (not the min-area proxy). The via_geom
# debug output pins the enclosure sizes, effective lengths and per-track demand
# on asap7, where every pair resolves to a default fixed via (no fallback).
source "helpers.tcl"
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def.gz

set_routing_layers -signal M2-M7
set_debug_level GRT via_geom 1

global_route -use_cugr
