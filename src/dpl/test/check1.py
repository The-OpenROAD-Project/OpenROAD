from openroad import Design, Tech
import helpers
import dpl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("simple01.def")
gpl = design.getOpendp()

try:
    gpl.checkPlacement(True)
except Exception as inst:
    print(inst.args[0])
