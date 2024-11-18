from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./incremental02.def")

gpl_aux.global_placement(design, incremental=True, density=0.3, pad_left=2, pad_right=2)

def_file = helpers.make_result_file("incremental02.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "incremental02.defok")
