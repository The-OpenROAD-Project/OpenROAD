import os.path
from openroad import Tech, Design
import helpers
import grt_aux

test_path = os.path.abspath(os.path.dirname(__file__))
tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = helpers.make_design(tech)
gcddef = os.path.join(test_path, "gcd.def")
design.readDef(gcddef)
gr = design.getGlobalRouter()

grt_aux.set_routing_layers(design, signal="metal2-metal10")

gr.setVerbose(True)
gr.globalRoute(True)

# FIXME Update when we have Python versions of the following
design.evalTclString("estimate_parasitics -global_routing")
design.evalTclString("report_net -digits 3 clk")
