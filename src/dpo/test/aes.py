from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("aes.def")

design.getOptdp().improvePlacement(1, 0, 0)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("aes.def")
design.writeDef(def_file)
helpers.diff_files("aes.defok", def_file)
