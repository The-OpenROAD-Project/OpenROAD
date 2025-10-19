# A hard blockage (to avoid) and a soft blockage (to ignore)
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("blockage01.def")

dpl_aux.detailed_placement(design)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("blockage01.def")
design.writeDef(def_file)
helpers.diff_files("blockage01.defok", def_file)
