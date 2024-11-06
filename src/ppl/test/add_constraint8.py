# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

design.evalTclString("set_io_pin_constraint -pin_names {req_msg*} -region bottom:0-18")
design.evalTclString(
    "set_io_pin_constraint -pin_names {resp_msg*} -region bottom:10-20"
)

ppl_aux.place_pins(design, hor_layers="metal3", ver_layers="metal2")

def_file = helpers.make_result_file("add_constraint8.def")
design.writeDef(def_file)
helpers.diff_files("add_constraint8.defok", def_file)
