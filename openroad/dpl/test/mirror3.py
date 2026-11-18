# optimize_mirroring without detailed placement
from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd_replace.def")
design.getOpendp().optimizeMirroring()
