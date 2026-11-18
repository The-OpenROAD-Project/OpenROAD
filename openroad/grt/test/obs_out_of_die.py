from openroad import Design, Tech
import helpers
import grt_aux

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("obs_out_of_die.def")
gr = design.getGlobalRouter()

guide_file = helpers.make_result_file("obs_out_of_die.guide")
grt_aux.set_routing_layers(design, signal="met1-met5")

gr.setVerbose(True)
gr.globalRoute(True)
design.getBlock().writeGuides(guide_file)

helpers.diff_files("obs_out_of_die.guideok", guide_file)
