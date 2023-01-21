from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./incremental02.def")

options = gpl.ReplaceOptions()
options.setTargetDensity(0.3)
options.setPadLeft(2)
options.setPadRight(2)
options.setIncremental(True)

design.getReplace().place(options)

def_file = helpers.make_result_file("incremental02.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "incremental02.defok")
