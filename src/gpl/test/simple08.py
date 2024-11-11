from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./sky130hd.lef")
design = helpers.make_design(tech)
design.readDef("./simple08.def")

gpl_aux.global_placement(design, density=0.75, bin_grid_count=64, overflow=0.2)

def_file = helpers.make_result_file("simple08.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple08.defok")
