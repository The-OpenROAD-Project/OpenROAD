# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("gcd.def")

design.evalTclString("set_io_pin_constraint -pin_names {resp_val resp_rdy req_rdy req_val req_msg*} -region bottom:*")
design.evalTclString("set_io_pin_constraint -pin_names {req_msg[15] req_msg[14] resp_msg[15] resp_msg[14]} -region top:*")

ppl_aux.place_pins(design, hor_layers="metal3", ver_layers="metal2", random=True)

def_file1 = helpers.make_result_file("add_constraint6_1.def")
design.writeDef(def_file1)

ppl_aux.clear_io_pin_constraints(design)

ppl_aux.place_pins(design, hor_layers="metal3", ver_layers="metal2")

def_file2 = helpers.make_result_file("add_constraint6_2.def")
design.writeDef(def_file2)

helpers.diff_files("add_constraint6_1.defok", def_file1)
helpers.diff_files("add_constraint6_2.defok", def_file2)
