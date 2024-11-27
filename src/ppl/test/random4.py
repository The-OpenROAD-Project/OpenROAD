# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.set_io_pin_constraint(
    design, pin_names="resp_val resp_rdy req_rdy req_val", region="bottom:*"
)
ppl_aux.set_io_pin_constraint(
    design,
    pin_names=r"req_msg\[15\] req_msg\[14\] resp_msg\[15\] resp_msg\[14\]",
    region="top:*",
)

ppl_aux.place_pins(design, hor_layers="metal3", ver_layers="metal2", random=True)

def_file = helpers.make_result_file("random4.def")
design.writeDef(def_file)
helpers.diff_files("random4.defok", def_file)
