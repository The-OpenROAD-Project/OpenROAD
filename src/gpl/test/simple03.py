from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple03.def")

gpl_aux.global_placement(design)

def_file = helpers.make_result_file("simple03.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple03.defok")
