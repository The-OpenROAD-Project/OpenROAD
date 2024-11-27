# exclude horizontal edges
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.set_io_pin_constraint(
    design, pin_names="resp_val resp_rdy req_rdy", group=True, order=True
)
ppl_aux.set_io_pin_constraint(
    design,
    pin_names="req_msg[15] req_msg[14] resp_msg[15] resp_msg[14]",
    group=True,
    order=True,
)

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file = helpers.make_result_file("group_pins9.def")
design.writeDef(def_file)
helpers.diff_files("group_pins9.defok", def_file)
