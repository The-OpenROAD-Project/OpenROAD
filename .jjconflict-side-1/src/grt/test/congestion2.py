import os.path
from openroad import Tech, Design
import helpers
import grt_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")
gr = design.getGlobalRouter()

guideFile = helpers.make_result_file("congestion2.guide")

grt_aux.set_global_routing_layer_adjustment(design, "metal2", 0.9)
grt_aux.set_global_routing_layer_adjustment(design, "metal3", 0.9)
grt_aux.set_global_routing_layer_adjustment(design, "metal4-metal6", 0.9)
grt_aux.set_global_routing_layer_adjustment(design, "metal7-metal10", 1.0)

grt_aux.set_routing_layers(design, signal="metal2-metal10")

gr.setVerbose(True)
gr.setAllowCongestion(True)
gr.globalRoute(True)

design.getBlock().writeGuides(guideFile)

diff = helpers.diff_files("congestion2.guideok", guideFile)
