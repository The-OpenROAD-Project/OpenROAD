from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple01-ref.def")

options = gpl.ReplaceOptions()
options.setReferenceHpwl(384000)
options.setInitDensityPenalityFactor(0.01)
options.skipInitialPlace()

design.getReplace().place(options)

def_file = helpers.make_result_file("simple01-ref.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-ref.defok")
