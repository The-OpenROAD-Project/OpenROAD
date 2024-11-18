from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01.def")

gpl_aux.global_placement(design, init_density_penalty=0.01, skip_initial_place=True)

def_file = helpers.make_result_file("simple01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01.defok")
