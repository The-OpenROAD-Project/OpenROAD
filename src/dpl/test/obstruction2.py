# ram with obstruction
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLiberty("Nangate45/fakeram45_64x7.lib")
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = helpers.make_design(tech)
design.readDef("obstruction2.def")

dpl_aux.detailed_placement(design)
dpl_aux.filler_placement(design, masters=["FILLCELL.*"])
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("obstruction2.def")
design.writeDef(def_file)
helpers.diff_files("obstruction2.defok", def_file)
