# filler_placement with set_placement_padding
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("simple01.def")
dpl_aux.set_placement_padding(design, globl=True, left=2, right=2)

dpl_aux.detailed_placement(design)
dpl_aux.filler_placement(design, masters=["FILL.*"], verbose=True)
