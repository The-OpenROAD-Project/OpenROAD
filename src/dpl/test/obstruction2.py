# Check placement beside a macro that extends beyond the core.
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
design.getOpendp().checkPlacement(False)
