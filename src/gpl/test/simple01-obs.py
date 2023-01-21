from openroad import Design, Tech
import gpl
import helpers

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple01-obs.def")

options = gpl.ReplaceOptions()
options.setTargetDensity(0.8)
options.setInitDensityPenalityFactor(0.01)
options.skipInitialPlace()

design.getReplace().place(options)

def_file = helpers.make_result_file("simple01-obs.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-obs.defok")
