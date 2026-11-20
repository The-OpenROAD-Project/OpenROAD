from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("basic.def")

design.getExample().makeInstance("test")

def_file = helpers.make_result_file("basic.def")
design.writeDef(def_file)
helpers.diff_files("basic.defok", def_file)
