from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./sky130hd.lef")
design = Design(tech)
design.readDef("./simple07.def")

options = gpl.ReplaceOptions()
options.setTargetDensity(0.75)

design.getReplace().place(options)

def_file = helpers.make_result_file("simple07.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple07.defok")
