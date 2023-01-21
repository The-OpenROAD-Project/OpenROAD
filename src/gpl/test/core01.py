from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./core01.def")

options = gpl.ReplaceOptions()
options.setTargetDensity(0.6)
options.setInitDensityPenalityFactor(0.01)
options.skipInitialPlace()

design.getReplace().place(options)

def_file = helpers.make_result_file("core01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "core01.defok")
