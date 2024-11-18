from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("simple01.def")

dpl_aux.detailed_placement(design)
masters = ["FILLCELL_X1", "FILLCELL_X2"]
dpl_aux.filler_placement(design, masters=masters)
design.getOpendp().checkPlacement(False)
