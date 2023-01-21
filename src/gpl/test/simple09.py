from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple09.def")

options = gpl.ReplaceOptions()
options.setInitDensityPenalityFactor(1.0)
options.skipInitialPlace()

design.getReplace().place(options)

def_file = helpers.make_result_file("simple09.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple09.defok")
