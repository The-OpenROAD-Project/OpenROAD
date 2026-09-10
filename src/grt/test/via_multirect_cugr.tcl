# A via with several rects on one routing layer must have its full footprint
# (the union of the rects) charged, not just the last box visited. via_multirect
# defines VIA_C with two metal1 rects that union to 0.15x0.15; the via_geom line
# must report that unioned extent (encl lower=150x150), not 150x100 or 100x150.
source "helpers.tcl"
read_lef "via_multirect.lef"
read_def "via_priority.def"

set_routing_layers -signal metal1-metal2
set_debug_level GRT via_geom 1

global_route -use_cugr

set guide_file [make_result_file via_multirect_cugr.guide]
write_guides $guide_file
diff_file via_multirect_cugr.guideok $guide_file
