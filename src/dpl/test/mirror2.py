from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("extra.lef")
design = helpers.make_design(tech)
design.readDef("mirror2.def")

dpl_aux.set_placement_padding(design, globl=True, left=1, right=1)
dpl_aux.detailed_placement(design)

print("block orient " + design.getBlock().findInst("block1").getOrient(), flush=True)
design.getOpendp().optimizeMirroring()
print("block orient " + design.getBlock().findInst("block1").getOrient())
