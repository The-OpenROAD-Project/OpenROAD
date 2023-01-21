from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./incremental01.def")

options = gpl.ReplaceOptions()
options.setInitDensityPenalityFactor(0.1)
options.setIncremental(True)

design.getReplace().place(options)

def_file = helpers.make_result_file("incremental01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "incremental01.defok")
