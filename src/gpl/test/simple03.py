from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple03.def")

design.getReplace().place()

def_file = helpers.make_result_file("simple03.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple03.defok")
