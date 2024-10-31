# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.place_pins(
    design, hor_layers="metal3", ver_layers="metal2", random=True, random_seed=0
)

def_file = helpers.make_result_file("random2.def")
design.writeDef(def_file)
helpers.diff_files("random2.defok", def_file)
