from openroad import Design, Tech
import gpl_aux
import helpers

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01-obs.def")

# design.evalTclString("global_placement -init_density_penalty 0.01 -skip_initial_place -density 0.8")
gpl_aux.global_placement(
    design, init_density_penalty=0.01, skip_initial_place=True, density=0.8
)
def_file = helpers.make_result_file("simple01-obs.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-obs.defok")
