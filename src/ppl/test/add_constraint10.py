# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

design.evalTclString(
    "set_io_pin_constraint -pin_names {resp_msg[0] resp_msg[1] clk resp_val resp_rdy resp_msg[10] resp_msg[11] resp_msg[12] resp_msg[13]} -region bottom:*"
)
design.evalTclString(
    "set_io_pin_constraint -pin_names {resp_msg[3] resp_msg[2] resp_msg[14] req_val req_rdy req_msg[10] req_msg[11] req_msg[12] req_msg[13]} -region top:0-18"
)
design.evalTclString(
    "set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_msg[2] req_msg[2] resp_msg[3] req_msg[3]}"
)

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file = helpers.make_result_file("add_constraint10.def")
design.writeDef(def_file)
helpers.diff_files("add_constraint10.defok", def_file)
