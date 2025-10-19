# place pins at top layer
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
tech.readLef("blocked_region.lef")

design = helpers.make_design(tech)
design.readDef("blocked_region.def")

ppl_aux.define_pin_shape_pattern(
    design,
    layer_name="met5",
    x_step=6.8,
    y_step=6.8,
    region="50 50 250 250",
    size=[1.6, 2.5],
)

ppl_aux.set_io_pin_constraint(
    design,
    pin_names="clk resp_val req_val resp_rdy reset req_rdy",
    region="up:170 200 250 250",
)

ppl_aux.place_pins(design, hor_layers="met3", ver_layers="met2")

def_file = helpers.make_result_file("top_layer2.def")
design.writeDef(def_file)
helpers.diff_files("top_layer2.defok", def_file)
