from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd_replace.def")

dpl_aux.set_placement_padding(design, globl=True, left=1, right=1)
dpl_aux.detailed_placement(design)
design.getOpendp().optimizeMirroring()
