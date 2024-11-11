from openroad import Design, Tech
import helpers
import grt_aux

tech = Tech()
tech.readLef("macro_obs_not_aligned.lef")
design = helpers.make_design(tech)
design.readDef("macro_obs_not_aligned.def")
gr = design.getGlobalRouter()

guide_file = helpers.make_result_file("macro_obs_not_aligned.guide")

grt_aux.set_global_routing_layer_adjustment(design, "met1-met5", 0.7)
grt_aux.set_routing_layers(design, signal="met1-met5")

gr.setVerbose(True)
gr.globalRoute(True)

design.getBlock().writeGuides(guide_file)

helpers.diff_files("macro_obs_not_aligned.guideok", guide_file)
