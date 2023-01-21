from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./sky130hd.lef")
design = Design(tech)
design.readDef("./simple08.def")

options = gpl.ReplaceOptions()
options.setTargetDensity(0.75)
options.setBinGridCntX(64)
options.setBinGridCntY(64)
options.setTargetOverflow(0.2)

design.getReplace().place(options)

def_file = helpers.make_result_file("simple08.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple08.defok")
