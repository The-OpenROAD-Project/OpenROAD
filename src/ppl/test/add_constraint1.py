# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.set_io_pin_constraint(design, direction="INPUT", region="top:*")

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file = helpers.make_result_file("add_constraint1.def")
design.writeDef(def_file)
helpers.diff_files("add_constraint1.defok", def_file)
