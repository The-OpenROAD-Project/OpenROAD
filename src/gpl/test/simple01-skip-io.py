from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple01-skip-io.def")

options = gpl.ReplaceOptions()
options.setSkipIoMode(True)

design.getReplace().place(options)

def_file = helpers.make_result_file("simple01-skip-io.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-skip-io.defok")
