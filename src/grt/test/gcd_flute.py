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

guideFile = helpers.make_result_file("gcd_flute.guide")

design.evalTclString("set_routing_alpha 0.0")

gr.setVerbose(True)
gr.globalRoute(True)  #  save_guides = True

design.getBlock().writeGuides(guideFile)

diff = helpers.diff_files("gcd_flute.guideok", guideFile)
