from openroad import Design, Tech
import helpers
import gpl

tech = Tech()
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple04.def")

design.getReplace().place()

def_file = helpers.make_result_file("simple04.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple04.defok")
