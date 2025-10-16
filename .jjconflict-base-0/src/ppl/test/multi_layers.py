# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.place_pins(
    design,
    hor_layers="metal3 metal5",
    ver_layers="metal2 metal4",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file = helpers.make_result_file("multi_layers.def")
design.writeDef(def_file)
helpers.diff_files("multi_layers.defok", def_file)
