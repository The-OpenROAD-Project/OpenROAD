# set_padding -global -right
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("simple03.def")

dpl_aux.set_placement_padding(design, globl=True, right=5)

def_file = helpers.make_result_file("pad03.def")
design.writeDef(def_file)
helpers.diff_files("pad03.defok", def_file)
