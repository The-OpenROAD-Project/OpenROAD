from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01-ref.def")

gpl_aux.global_placement(
    design, init_density_penalty=0.01, skip_initial_place=True, reference_hpwl=384000.0
)

def_file = helpers.make_result_file("simple01-ref.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-ref.defok")
