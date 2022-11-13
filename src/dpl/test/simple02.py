# 1 inst in core off grid
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("simple02.def")

dpl_aux.detailed_placement(design)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("simple02.def")
design.writeDef(def_file)
helpers.diff_files("simple02.defok", def_file)
