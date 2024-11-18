# corner pad/endcaps with obstructions
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("obstruction1.lef")
design = helpers.make_design(tech)
design.readDef("obstruction1.def")

dpl_aux.detailed_placement(design)
dpl_aux.filler_placement(design, masters=["FILLCELL.*"])
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("obstruction1.def")
design.writeDef(def_file)
helpers.diff_files("obstruction1.defok", def_file)
