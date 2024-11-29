import os.path
from openroad import Tech, Design
import helpers
import grt_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("region_adjustment.def")

gr = design.getGlobalRouter()

llx = design.micronToDBU(1.4)
lly = design.micronToDBU(2.0)
urx = design.micronToDBU(20.0)
ury = design.micronToDBU(15.5)

gr.addRegionAdjustment(llx, lly, urx, ury, 2, 0.9)

guideFile = helpers.make_result_file("region_adjustment.guide")

gr.setVerbose(True)
gr.globalRoute(True)  # save_guides = True

design.getBlock().writeGuides(guideFile)

diff = helpers.diff_files("region_adjustment.guideok", guideFile)
