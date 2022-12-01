from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("simple01.def")

dpl_aux.detailed_placement(design)
# In the tcl version, the regex "FILLCELL_X1" matches both "FILLCELL_X1" and 
# "FILLCELL_X16" but here we must specify both
masters=["FILLCELL_X1", "FILLCELL_X16", "FILLCELL_X2"]
dpl_aux.filler_placement(design, masters=masters)
design.getOpendp().checkPlacement(False)
