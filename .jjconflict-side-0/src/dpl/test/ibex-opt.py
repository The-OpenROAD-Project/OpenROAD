from openroad import Design, Tech
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("ibex-opt.def")

design.getOpendp().improvePlacement(1, 0, 0)
design.getOpendp().checkPlacement(False)

def_file = helpers.make_result_file("ibex-opt.def")
design.writeDef(def_file)
helpers.diff_files("ibex-opt.defok", def_file)
