# filler_placement with verbose
from openroad import Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("fillers9.lef")
design = helpers.make_design(tech)
design.readDef("fillers9.def")

dpl_aux.filler_placement(design, masters=["FILL.*"], verbose=True)
