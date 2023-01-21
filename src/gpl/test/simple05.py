from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple05.def")

options = gpl.ReplaceOptions()
options.skipInitialPlace()

design.getReplace().place(options)

def_file = helpers.make_result_file("simple05.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple05.defok")
