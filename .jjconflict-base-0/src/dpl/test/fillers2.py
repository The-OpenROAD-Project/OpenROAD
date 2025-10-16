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
dpl_aux.filler_placement(design, masters=["FILL.*"])

# disable padding for placement checks
dpl_aux.set_placement_padding(design, globl=True, left=0, right=0)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("fillers2.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "fillers2.defok")
