# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.define_pin_shape_pattern(
    design,
    layer_name="metal10",
    x_step=4.8,
    y_step=4.8,
    region="0.095 0.07 90 90",
    size=[1.6, 2.5],
)

ppl_aux.set_io_pin_constraint(
    design,
    pin_names="clk resp_val req_val resp_rdy reset req_rdy",
    region="up:70 50 75 80",
)

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file = helpers.make_result_file("top_layer3.def")
design.writeDef(def_file)
helpers.diff_files("top_layer3.defok", def_file)
