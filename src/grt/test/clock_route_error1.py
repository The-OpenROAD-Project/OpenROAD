from openroad import Tech, Design
import helpers
import grt_aux

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("clock_route.def")

# FIXME: When stt has been Python wrapped, use the Python version of set_routing_alpha
design.evalTclString("set_routing_alpha 2")
print("STT-0001")
