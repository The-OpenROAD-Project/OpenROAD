from openroad import Design, Tech
import helpers
import grt_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("multiple_calls.def")

guide_file1 = helpers.make_result_file("mc1_route.guide")
guide_file2 = helpers.make_result_file("mc2_route.guide")

gr = design.getGlobalRouter()

gr.setVerbose(True)
gr.setGridOrigin(0, 0)
gr.globalRoute(True)
design.getBlock().writeGuides(guide_file1)

grt_aux.set_global_routing_layer_adjustment(design, "*", 0.8)
gr.setVerbose(True)
gr.globalRoute(True)
design.getBlock().writeGuides(guide_file2)

helpers.diff_files("multiple_calls.guideok1", guide_file1)
helpers.diff_files("multiple_calls.guideok2", guide_file2)
