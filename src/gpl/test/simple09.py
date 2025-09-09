from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple09.def")

gpl_aux.global_placement(design, init_density_penalty=1.0, skip_initial_place=True)

def_file = helpers.make_result_file("simple09.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple09.defok")
