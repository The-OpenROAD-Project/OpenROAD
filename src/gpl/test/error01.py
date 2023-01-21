from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./error01.def")

try:
    options = gpl.ReplaceOptions()
    options.setTargetDensity(0.001)
    options.setInitDensityPenalityFactor(0.01)
    options.skipInitialPlace()

    design.getReplace().place(options)
except Exception as inst:
    print(inst.args[0])

