# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
from openroad import Design, Tech
import helpers
import grt_aux

tech = Tech()
tech.readLiberty("sky130hs/sky130hs_tt.lib")
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")

design = helpers.make_design(tech)
design.readDef("tracks1.def")
gr = design.getGlobalRouter()

guide_file = helpers.make_result_file("tracks1.guide")

gr.setVerbose(True)
gr.globalRoute(True)

design.getBlock().writeGuides(guide_file)

helpers.diff_files("tracks1.guideok", guide_file)
