from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("fill3.lef")
design = Design(tech)
design.readDef("fillers4.def")

dpl_aux.detailed_placement(design, suppress=True)
masters=["FILLCELL_X1", "FILLCELL_X16", "FILLCELL_X2", "FILLCELL_X3", "FILLCELL_X4", "FILLCELL_X8"]
dpl_aux.filler_placement(design, masters=masters)
design.getOpendp().checkPlacement(False)
