from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./error01.def")

try:
    gpl_aux.global_placement(
        design, init_density_penalty=0.01, skip_initial_place=True, density=0.001
    )
except Exception as inst:
    print(inst.args[0])
