import os.path
from openroad import Tech, Design
import helpers
import grt_aux

test_path = os.path.abspath(os.path.dirname(__file__))
tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
gcddef = os.path.join(test_path, "gcd.def")
design.readDef(gcddef)
gr = design.getGlobalRouter()

guideFile = helpers.make_result_file("congestion1.guide")

grt_aux.set_global_routing_layer_adjustment(design, "metal2", 0.9)
grt_aux.set_global_routing_layer_adjustment(design, "metal3", 0.9)
grt_aux.set_global_routing_layer_adjustment(design, "metal4-metal10", 1.0)

grt_aux.set_routing_layers(design, signal="metal2-metal10")

gr.setVerbose(True)
gr.setAllowCongestion(True)
gr.globalRoute(True)  # save_guides = True

design.getBlock().writeGuides(guideFile)

diff = helpers.diff_files("congestion1.guideok", guideFile)
