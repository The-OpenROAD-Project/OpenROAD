from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./incremental01.def")

gpl_aux.global_placement(design, init_density_penalty=0.1, incremental=True)

def_file = helpers.make_result_file("incremental01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "incremental01.defok")
