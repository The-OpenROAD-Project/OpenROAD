# set_placement_padding -global -left
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("simple01.def")

dpl_aux.set_placement_padding(design, globl=True, left=5)
dpl_aux.detailed_placement(design)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("pad01.def")
design.writeDef(def_file)
helpers.diff_files("pad01.defok", def_file)
